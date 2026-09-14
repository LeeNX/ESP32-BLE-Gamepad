#ifndef BleOTA_h
#define BleOTA_h

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// GATT protocol reference: BleOTA.md. UUIDs below are specific to this
// library -- don't reuse them for an unrelated service.
#define OTA_SERVICE_UUID "56d794fe-92e6-4227-8570-08eb990b4648"
#define OTA_CONTROL_CHARACTERISTIC_UUID "4a0b7397-e27a-40c6-b8a0-2ae63eb2d5df"
#define OTA_DATA_CHARACTERISTIC_UUID "28afb7ac-af3e-4e53-a58f-2fef9e5ad51b"
#define OTA_STATUS_CHARACTERISTIC_UUID "f2c7832e-1e92-42b2-8d84-540b7db3979b"

// Control characteristic opcodes -- one byte opcode followed by an
// opcode-specific payload. See BleOTA.md for the exact wire format.
enum BleOTAOpcode : uint8_t
{
    OTA_OP_BEGIN = 0x01,
    OTA_OP_END = 0x03,
    OTA_OP_REBOOT = 0x04,
    OTA_OP_ABORT = 0x05,
    OTA_OP_CONFIRM_VALID = 0x06,
    OTA_OP_ENTER_RECOVERY = 0x07,
    OTA_OP_GET_INFO = 0x08,
};

// Status characteristic notification states -- one byte state followed by a
// state-specific payload.
enum BleOTAState : uint8_t
{
    OTA_STATE_IDLE = 0,
    OTA_STATE_RECEIVING = 1,
    OTA_STATE_VALIDATING = 2,
    OTA_STATE_READY_TO_REBOOT = 3,
    OTA_STATE_REBOOTING = 4,
    OTA_STATE_ERROR = 5,
    OTA_STATE_INFO = 6,
    OTA_STATE_CONFIRMED = 7,
};

enum BleOTAError : uint8_t
{
    OTA_ERR_SIZE_MISMATCH = 1,
    OTA_ERR_HASH_MISMATCH = 2,
    OTA_ERR_FLASH_WRITE_FAILED = 3,
    OTA_ERR_NO_FREE_PARTITION = 4,
    OTA_ERR_ALREADY_IN_PROGRESS = 5,
    OTA_ERR_NOT_STARTED = 6,
    OTA_ERR_IMAGE_INVALID = 7,
    OTA_ERR_TOO_LARGE = 8,
    OTA_ERR_BAD_REQUEST = 9,
};

// Largest single BLE write this class accepts, on either characteristic.
// Comfortably above any MTU (up to 517) this library negotiates.
#define BLE_OTA_MAX_CHUNK 512
#define BLE_OTA_QUEUE_DEPTH 8
#define BLE_OTA_RECOVERY_MAGIC 0x4F544152UL // "OTAR"

class BleOTA : public NimBLECharacteristicCallbacks
{
public:
    BleOTA(NimBLEServer *existingServer);
    ~BleOTA();

    void begin();
    void end();

    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override;
    void onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue) override;

private:
    struct QueueItem
    {
        bool isControl;
        uint16_t length;
        uint8_t data[BLE_OTA_MAX_CHUNK];
    };

    NimBLEServer *pServer;
    NimBLEService *pService;
    NimBLECharacteristic *pControlCharacteristic;
    NimBLECharacteristic *pDataCharacteristic;
    NimBLECharacteristic *pStatusCharacteristic;

    // onWrite() for both characteristics only copies the payload into this
    // queue and returns immediately -- esp_ota_begin()/esp_ota_write() do
    // sector erase+program (multi-ms) and must never run inline inside the
    // NimBLE host's callback context, or they risk stalling BLE connection
    // timing. writerTask drains the queue on its own FreeRTOS task, so
    // Control opcodes and Data chunks are processed strictly in the order
    // the peer sent them (a single BLE link preserves per-characteristic
    // write ordering).
    QueueHandle_t chunkQueue;
    TaskHandle_t writerTaskHandle;
    static void writerTaskTrampoline(void *self);
    void writerTask();

    // Update-in-progress state. Touched only from writerTask()'s thread.
    esp_ota_handle_t otaHandle;
    const esp_partition_t *targetPartition;
    uint32_t expectedSize;
    uint32_t bytesWritten;
    uint8_t expectedSha256[32];
    mbedtls_sha256_context shaCtx;
    bool updateInProgress;
    uint16_t negotiatedMTU;

    void handleControl(const uint8_t *data, size_t length);
    void handleData(const uint8_t *data, size_t length);
    void resetUpdateState();
    void sendStatus(uint8_t state, const uint8_t *payload = nullptr, size_t payloadLength = 0);
    void sendError(uint8_t errorCode);
    void sendInfo();
};

#endif
