#include "BleOTA.h"
#include "BleOTARollback.h"

#include <cstring>
#include <esp_partition.h>

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif

BleOTA::BleOTA(NimBLEServer *existingServer)
    : pServer(existingServer), pService(nullptr), pControlCharacteristic(nullptr), pDataCharacteristic(nullptr),
      pStatusCharacteristic(nullptr), chunkQueue(nullptr), writerTaskHandle(nullptr), otaHandle(0),
      targetPartition(nullptr), expectedSize(0), bytesWritten(0), updateInProgress(false), negotiatedMTU(23)
{
    memset(expectedSha256, 0, sizeof(expectedSha256));
}

BleOTA::~BleOTA()
{
    end();
}

void BleOTA::begin()
{
    delay(1000); // Give other services (HID, NUS) time to finish their own setup first

    if (!pServer)
    {
        Serial.println("[OTA] No existing pServer available");
        return;
    }

    chunkQueue = xQueueCreate(BLE_OTA_QUEUE_DEPTH, sizeof(QueueItem));
    xTaskCreate(writerTaskTrampoline, "BleOTAWriter", 8192, this, 1, &writerTaskHandle);

    NimBLEDevice::setMTU(517); // largest usable MTU -- actual negotiated value read back per-connection via connInfo.getMTU()

    NimBLEAdvertising *pAdvertising = pServer->getAdvertising();
    Serial.println("[OTA] Stopping main NimBLE server from advertising (shouldn't be at this stage if you set delayAdvertising to true)");
    pAdvertising->stop();

    Serial.println("[OTA] Creating BLE OTA service");
    pService = pServer->createService(OTA_SERVICE_UUID);

    pControlCharacteristic = pService->createCharacteristic(
        OTA_CONTROL_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
    pDataCharacteristic = pService->createCharacteristic(
        OTA_DATA_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::WRITE_ENC);
    pStatusCharacteristic = pService->createCharacteristic(OTA_STATUS_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY);

    // Control/Data require an encrypted, bonded link (WRITE_ENC) -- OTA write
    // access is "anyone nearby can brick or reflash the device," unlike the
    // open BleNUS service. The first write from an unbonded peer makes the
    // NimBLE host trigger the standard pairing flow (BleGamepad already
    // enables bonding via NimBLEDevice::setSecurityAuth(true, false, false)).
    pControlCharacteristic->setCallbacks(this);
    pDataCharacteristic->setCallbacks(this);
    pStatusCharacteristic->setCallbacks(this); // logs subscribe/unsubscribe only

    // Can't add the OTA service UUID to the main 31-byte advertisement
    // alongside the HID service UUID -- same constraint BleNUS documents.
    // Put it in the scan response instead.
    NimBLEAdvertisementData scanResponseData;
    scanResponseData.addServiceUUID(pService->getUUID());
    pAdvertising->setScanResponseData(scanResponseData);

    Serial.println("[OTA] Main NimBLE server advertising started!");
    pAdvertising->start();
}

void BleOTA::end()
{
    if (writerTaskHandle)
    {
        vTaskDelete(writerTaskHandle);
        writerTaskHandle = nullptr;
    }
    if (chunkQueue)
    {
        vQueueDelete(chunkQueue);
        chunkQueue = nullptr;
    }
    if (updateInProgress)
    {
        esp_ota_abort(otaHandle);
        resetUpdateState();
    }
}

void BleOTA::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
{
    negotiatedMTU = connInfo.getMTU();

    if (pCharacteristic != pControlCharacteristic && pCharacteristic != pDataCharacteristic)
        return;

    std::string value = pCharacteristic->getValue();
    if (value.empty())
        return;
    if (value.length() > BLE_OTA_MAX_CHUNK)
    {
        sendError(OTA_ERR_BAD_REQUEST);
        return;
    }

    QueueItem item;
    item.isControl = (pCharacteristic == pControlCharacteristic);
    item.length = (uint16_t)value.length();
    memcpy(item.data, value.data(), item.length);

    if (!chunkQueue || xQueueSend(chunkQueue, &item, 0) != pdTRUE)
    {
        Serial.println("[OTA] writer queue full -- dropping write");
        sendError(OTA_ERR_FLASH_WRITE_FAILED);
    }
}

void BleOTA::onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue)
{
    negotiatedMTU = connInfo.getMTU();
    const char *action = subValue == 0 ? "unsubscribed from" : "subscribed to";
    Serial.printf("[OTA] %s %s the OTA status characteristic\n", std::string(connInfo.getAddress()).c_str(), action);
}

void BleOTA::writerTaskTrampoline(void *self)
{
    static_cast<BleOTA *>(self)->writerTask();
}

void BleOTA::writerTask()
{
    QueueItem item;
    for (;;)
    {
        if (xQueueReceive(chunkQueue, &item, portMAX_DELAY) != pdTRUE)
            continue;

        if (item.isControl)
            handleControl(item.data, item.length);
        else
            handleData(item.data, item.length);
    }
}

void BleOTA::handleControl(const uint8_t *data, size_t length)
{
    if (length < 1)
    {
        sendError(OTA_ERR_BAD_REQUEST);
        return;
    }

    uint8_t opcode = data[0];
    switch (opcode)
    {
        case OTA_OP_BEGIN:
        {
            if (updateInProgress)
            {
                sendError(OTA_ERR_ALREADY_IN_PROGRESS);
                return;
            }
            // uint32 imageSize, uint8 sha256[32], uint8 versionLen, char version[versionLen]
            if (length < 1 + 4 + 32 + 1)
            {
                sendError(OTA_ERR_BAD_REQUEST);
                return;
            }

            uint32_t imageSize;
            memcpy(&imageSize, data + 1, 4);

            const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
            if (!part)
            {
                sendError(OTA_ERR_NO_FREE_PARTITION);
                return;
            }
            if (imageSize == 0 || imageSize > part->size)
            {
                sendError(OTA_ERR_TOO_LARGE);
                return;
            }

            esp_err_t err = esp_ota_begin(part, imageSize, &otaHandle);
            if (err != ESP_OK)
            {
                Serial.printf("[OTA] esp_ota_begin failed: %s\n", esp_err_to_name(err));
                sendError(OTA_ERR_FLASH_WRITE_FAILED);
                return;
            }

            targetPartition = part;
            expectedSize = imageSize;
            bytesWritten = 0;
            memcpy(expectedSha256, data + 5, 32);
            mbedtls_sha256_init(&shaCtx);
            mbedtls_sha256_starts(&shaCtx, 0); // 0 == SHA-256 (not the SHA-224 variant)
            updateInProgress = true;

            Serial.printf("[OTA] BEGIN: %u bytes -> %s\n", (unsigned)imageSize, part->label);
            uint8_t payload[8];
            memcpy(payload, &bytesWritten, 4);
            memcpy(payload + 4, &expectedSize, 4);
            sendStatus(OTA_STATE_RECEIVING, payload, sizeof(payload));
            break;
        }

        case OTA_OP_END:
        {
            if (!updateInProgress)
            {
                sendError(OTA_ERR_NOT_STARTED);
                return;
            }
            if (bytesWritten != expectedSize)
            {
                sendError(OTA_ERR_SIZE_MISMATCH);
                esp_ota_abort(otaHandle);
                resetUpdateState();
                return;
            }

            uint8_t actualSha256[32];
            mbedtls_sha256_finish(&shaCtx, actualSha256);
            if (memcmp(actualSha256, expectedSha256, 32) != 0)
            {
                Serial.println("[OTA] SHA-256 mismatch");
                sendError(OTA_ERR_HASH_MISMATCH);
                esp_ota_abort(otaHandle);
                resetUpdateState();
                return;
            }

            sendStatus(OTA_STATE_VALIDATING);
            esp_err_t err = esp_ota_end(otaHandle);
            if (err != ESP_OK)
            {
                Serial.printf("[OTA] esp_ota_end failed: %s\n", esp_err_to_name(err));
                sendError(OTA_ERR_IMAGE_INVALID);
                resetUpdateState();
                return;
            }

            Serial.println("[OTA] Image validated, ready to reboot");
            updateInProgress = false; // done writing, but keep targetPartition for REBOOT
            sendStatus(OTA_STATE_READY_TO_REBOOT);
            break;
        }

        case OTA_OP_REBOOT:
        {
            if (!targetPartition)
            {
                sendError(OTA_ERR_NOT_STARTED);
                return;
            }
            esp_err_t err = esp_ota_set_boot_partition(targetPartition);
            if (err != ESP_OK)
            {
                Serial.printf("[OTA] esp_ota_set_boot_partition failed: %s\n", esp_err_to_name(err));
                sendError(OTA_ERR_FLASH_WRITE_FAILED);
                return;
            }
            Serial.println("[OTA] Rebooting into new image");
            sendStatus(OTA_STATE_REBOOTING);
            delay(500); // let the notify flush before the link drops
            esp_restart();
            break;
        }

        case OTA_OP_ABORT:
        {
            if (updateInProgress)
                esp_ota_abort(otaHandle);
            resetUpdateState();
            sendStatus(OTA_STATE_IDLE);
            break;
        }

        case OTA_OP_CONFIRM_VALID:
        {
            BleOTARollback::confirmValid();
            sendStatus(OTA_STATE_CONFIRMED);
            break;
        }

        case OTA_OP_ENTER_RECOVERY:
        {
            if (length < 1 + 4)
            {
                sendError(OTA_ERR_BAD_REQUEST);
                return;
            }
            uint32_t magic;
            memcpy(&magic, data + 1, 4);
            if (magic != BLE_OTA_RECOVERY_MAGIC)
            {
                sendError(OTA_ERR_BAD_REQUEST);
                return;
            }
            const esp_partition_t *factory =
                esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
            if (!factory)
            {
                sendError(OTA_ERR_NO_FREE_PARTITION);
                return;
            }
            esp_ota_set_boot_partition(factory);
            Serial.println("[OTA] ENTER_RECOVERY -- rebooting into factory");
            sendStatus(OTA_STATE_REBOOTING);
            delay(500);
            esp_restart();
            break;
        }

        case OTA_OP_GET_INFO:
            sendInfo();
            break;

        default:
            sendError(OTA_ERR_BAD_REQUEST);
            break;
    }
}

void BleOTA::handleData(const uint8_t *data, size_t length)
{
    if (!updateInProgress)
    {
        sendError(OTA_ERR_NOT_STARTED);
        return;
    }
    if ((uint64_t)bytesWritten + length > expectedSize)
    {
        sendError(OTA_ERR_SIZE_MISMATCH);
        esp_ota_abort(otaHandle);
        resetUpdateState();
        return;
    }

    esp_err_t err = esp_ota_write(otaHandle, data, length);
    if (err != ESP_OK)
    {
        Serial.printf("[OTA] esp_ota_write failed: %s\n", esp_err_to_name(err));
        sendError(OTA_ERR_FLASH_WRITE_FAILED);
        esp_ota_abort(otaHandle);
        resetUpdateState();
        return;
    }

    mbedtls_sha256_update(&shaCtx, data, length);
    bytesWritten += length;

    uint8_t payload[8];
    memcpy(payload, &bytesWritten, 4);
    memcpy(payload + 4, &expectedSize, 4);
    sendStatus(OTA_STATE_RECEIVING, payload, sizeof(payload));
}

void BleOTA::resetUpdateState()
{
    updateInProgress = false;
    otaHandle = 0;
    targetPartition = nullptr;
    expectedSize = 0;
    bytesWritten = 0;
    memset(expectedSha256, 0, sizeof(expectedSha256));
}

void BleOTA::sendStatus(uint8_t state, const uint8_t *payload, size_t payloadLength)
{
    if (!pStatusCharacteristic)
        return;
    if (pServer->getConnectedCount() == 0)
        return;

    uint8_t buf[64];
    buf[0] = state;
    size_t total = 1;
    if (payload && payloadLength)
    {
        size_t n = payloadLength < sizeof(buf) - 1 ? payloadLength : sizeof(buf) - 1;
        memcpy(buf + 1, payload, n);
        total += n;
    }
    pStatusCharacteristic->setValue(buf, total);
    pStatusCharacteristic->notify();
}

void BleOTA::sendError(uint8_t errorCode)
{
    sendStatus(OTA_STATE_ERROR, &errorCode, 1);
}

void BleOTA::sendInfo()
{
    std::string label = BleOTARollback::getRunningSlotLabel();

    uint8_t payload[24] = {0};
    size_t labelLen = label.length() < 16 ? label.length() : 16;
    memcpy(payload, label.data(), labelLen); // [0..15] running slot label, NUL-padded
    payload[16] = BleOTARollback::isRunningSlotConfirmed() ? 1 : 0;
    payload[17] = BleOTARollback::getBootAttemptCount();
    memcpy(payload + 18, &negotiatedMTU, 2); // [18..19] negotiated ATT MTU, little-endian
    // [20..23] reserved

    sendStatus(OTA_STATE_INFO, payload, sizeof(payload));
}
