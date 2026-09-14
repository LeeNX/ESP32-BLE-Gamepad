#include "BleOTARollback.h"

#include <cstring>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <nvs.h>
#include <nvs_flash.h>

// NVS keys are capped at 15 characters, so key names are kept to a
// "<prefix>_<partition label>" shape (labels here are "factory"/"ota_0"/
// "ota_1", the longest being 7 chars -- "cfm_factory" is 11, well under the
// limit).
#define OTA_ROLLBACK_NVS_NAMESPACE "ble_ota"
#define OTA_ROLLBACK_SHA_LEN 8 // truncated esp_app_desc_t.app_elf_sha256 -- just enough to tell images apart

int8_t BleOTARollback::recoveryButtonPin = -1;

static uint8_t s_bootAttemptCount = 0;

static const esp_partition_t *findAppPartition(esp_partition_subtype_t subtype)
{
    return esp_partition_find_first(ESP_PARTITION_TYPE_APP, subtype, NULL);
}

static bool ensureNvsReady()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // Whole NVS partition is unusable (first boot after a layout change,
        // or truncated/corrupt) -- wipe it and retry once. There's no OTA
        // history worth preserving if this happens.
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return err == ESP_OK;
}

static void keyFor(const char *prefix, const std::string &label, char *out, size_t outSize)
{
    snprintf(out, outSize, "%s_%s", prefix, label.c_str());
}

static bool readImageSha(const esp_partition_t *part, uint8_t sha[32])
{
    esp_app_desc_t desc;
    if (esp_ota_get_partition_description(part, &desc) != ESP_OK)
        return false;
    memcpy(sha, desc.app_elf_sha256, 32);
    return true;
}

static bool isConfirmed(nvs_handle_t h, const std::string &label)
{
    char key[16];
    keyFor("cfm", label, key, sizeof(key));
    uint8_t confirmed = 0;
    nvs_get_u8(h, key, &confirmed); // defaults to 0 (not confirmed) if the key doesn't exist yet
    return confirmed != 0;
}

void BleOTARollback::setRecoveryButtonPin(int8_t gpio)
{
    recoveryButtonPin = gpio;
}

std::string BleOTARollback::getRunningSlotLabel()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    return running ? std::string(running->label) : std::string();
}

uint8_t BleOTARollback::getBootAttemptCount()
{
    return s_bootAttemptCount;
}

bool BleOTARollback::isRunningSlotConfirmed()
{
    if (!ensureNvsReady())
        return false;

    nvs_handle_t h;
    if (nvs_open(OTA_ROLLBACK_NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK)
        return false;

    bool confirmed = isConfirmed(h, getRunningSlotLabel());
    nvs_close(h);
    return confirmed;
}

void BleOTARollback::checkAndHandleRollback(uint8_t maxAttempts)
{
    // Physical recovery override: a held boot button always wins, regardless
    // of how healthy the running image otherwise looks.
    if (recoveryButtonPin >= 0)
    {
        pinMode(recoveryButtonPin, INPUT_PULLUP);
        if (digitalRead(recoveryButtonPin) == LOW)
        {
            const esp_partition_t *factory = findAppPartition(ESP_PARTITION_SUBTYPE_APP_FACTORY);
            const esp_partition_t *running = esp_ota_get_running_partition();
            if (factory && running != factory)
            {
                Serial.println("[OTA-Rollback] Recovery button held -- booting factory partition");
                esp_ota_set_boot_partition(factory);
                esp_restart();
            }
        }
    }

    if (!ensureNvsReady())
    {
        Serial.println("[OTA-Rollback] NVS unavailable -- skipping rollback bookkeeping");
        return;
    }

    nvs_handle_t h;
    if (nvs_open(OTA_ROLLBACK_NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK)
    {
        Serial.println("[OTA-Rollback] Failed to open NVS namespace -- skipping rollback bookkeeping");
        return;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    std::string label = running ? std::string(running->label) : std::string("unknown");

    uint8_t sha[32] = {0};
    bool haveSha = running && readImageSha(running, sha);

    char shaKey[16], cfmKey[16], cntKey[16];
    keyFor("sha", label, shaKey, sizeof(shaKey));
    keyFor("cfm", label, cfmKey, sizeof(cfmKey));
    keyFor("cnt", label, cntKey, sizeof(cntKey));

    uint8_t storedSha[OTA_ROLLBACK_SHA_LEN] = {0};
    size_t storedShaLen = sizeof(storedSha);
    esp_err_t shaErr = nvs_get_blob(h, shaKey, storedSha, &storedShaLen);
    bool isNewImage = !haveSha || shaErr != ESP_OK || storedShaLen != OTA_ROLLBACK_SHA_LEN ||
                      memcmp(storedSha, sha, OTA_ROLLBACK_SHA_LEN) != 0;

    uint8_t cnt = 0;
    if (isNewImage)
    {
        cnt = 1;
        nvs_set_blob(h, shaKey, sha, OTA_ROLLBACK_SHA_LEN);
        nvs_set_u8(h, cfmKey, 0);
        nvs_set_u8(h, cntKey, cnt);
        Serial.printf("[OTA-Rollback] New image detected in '%s' -- boot attempt 1/%u\n", label.c_str(), maxAttempts);
    }
    else if (isConfirmed(h, label))
    {
        cnt = 0; // confirmed builds don't accumulate attempts
        Serial.printf("[OTA-Rollback] '%s' is already confirmed -- normal boot\n", label.c_str());
    }
    else
    {
        nvs_get_u8(h, cntKey, &cnt);
        cnt++;
        nvs_set_u8(h, cntKey, cnt);
        Serial.printf("[OTA-Rollback] '%s' still unconfirmed -- boot attempt %u/%u\n", label.c_str(), cnt, maxAttempts);
    }
    nvs_commit(h);
    s_bootAttemptCount = cnt;

    bool needsRollback = cnt > maxAttempts;
    nvs_close(h);

    if (!needsRollback)
        return;

    Serial.printf("[OTA-Rollback] '%s' exceeded %u unconfirmed boots -- rolling back\n", label.c_str(), maxAttempts);

    // Best effort: let the bootloader do it natively if it was built with
    // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE. This only returns on failure --
    // on success the device is already rebooting into the previous app.
    esp_err_t rc = esp_ota_mark_app_invalid_rollback_and_reboot();
    Serial.printf("[OTA-Rollback] Native rollback unavailable (%s) -- falling back to manual slot selection\n", esp_err_to_name(rc));

    // Manual fallback: pick the other OTA slot if it's confirmed-good,
    // otherwise fall back all the way to the never-OTA-written factory slot.
    const esp_partition_t *other = nullptr;
    if (running)
    {
        if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0)
            other = findAppPartition(ESP_PARTITION_SUBTYPE_APP_OTA_1);
        else if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1)
            other = findAppPartition(ESP_PARTITION_SUBTYPE_APP_OTA_0);
        // If running is already `factory`, there's nowhere further to fall
        // back to -- factory is the terminal case, so `other` stays null and
        // we just continue booting it below.
    }

    const esp_partition_t *target = nullptr;
    if (other)
    {
        nvs_handle_t h2;
        if (nvs_open(OTA_ROLLBACK_NVS_NAMESPACE, NVS_READWRITE, &h2) == ESP_OK)
        {
            if (isConfirmed(h2, std::string(other->label)))
                target = other;
            nvs_close(h2);
        }
    }
    if (!target)
        target = findAppPartition(ESP_PARTITION_SUBTYPE_APP_FACTORY);

    if (target && target != running)
    {
        Serial.printf("[OTA-Rollback] Booting '%s' instead\n", target->label);
        esp_ota_set_boot_partition(target);
        esp_restart();
    }
    else
    {
        Serial.println("[OTA-Rollback] No better slot available -- continuing to boot the current image");
    }
}

void BleOTARollback::confirmValid()
{
    if (!ensureNvsReady())
        return;

    nvs_handle_t h;
    if (nvs_open(OTA_ROLLBACK_NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK)
        return;

    std::string label = getRunningSlotLabel();
    char cfmKey[16], cntKey[16];
    keyFor("cfm", label, cfmKey, sizeof(cfmKey));
    keyFor("cnt", label, cntKey, sizeof(cntKey));

    nvs_set_u8(h, cfmKey, 1);
    nvs_set_u8(h, cntKey, 0);
    nvs_commit(h);
    nvs_close(h);
    s_bootAttemptCount = 0;

    esp_err_t rc = esp_ota_mark_app_valid_cancel_rollback(); // harmless no-op (ESP_ERR_NOT_SUPPORTED) if native rollback isn't compiled in
    Serial.printf("[OTA-Rollback] '%s' confirmed valid (native rollback cancel: %s)\n", label.c_str(), esp_err_to_name(rc));
}
