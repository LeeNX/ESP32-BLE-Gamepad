#ifndef BleOTARollback_h
#define BleOTARollback_h

#include <Arduino.h>
#include <string>

// Boot-time OTA rollback state machine. No NimBLE/BLE dependency at all --
// safe (and required) to call before any BLE/HID initialisation.
//
// Pairs with BleOTA (the in-session BLE flashing service): BleOTA writes a
// new image into the free ota_0/ota_1 slot and reboots into it; this class
// runs on every boot, decides whether the currently-running slot is a
// still-on-probation image, and falls back to the previous slot (then to
// `factory`) if it never gets confirmed within maxAttempts boots.
//
// Deliberately does not rely on the bootloader's own CONFIG_BOOTLOADER_APP_
// ROLLBACK_ENABLE rollback support being compiled in (it varies across
// arduino-esp32 core versions/boards): checkAndHandleRollback() tries the
// native esp_ota_mark_app_invalid_rollback_and_reboot() first, and falls
// back to an NVS-backed boot-attempt counter + esp_ota_set_boot_partition()
// of its own when that isn't supported.
class BleOTARollback
{
public:
    // Call as the literal first statement in setup(), before Serial.begin(),
    // before constructing BleGamepad, before anything that could plausibly
    // crash. Detects whether the running partition is a freshly-flashed,
    // not-yet-confirmed image; if it's been booted (without a confirmValid()
    // call) more than maxAttempts times, rolls back to the other OTA slot
    // (or to `factory` if that slot isn't confirmed-good either) and
    // reboots. Returns normally if no rollback was needed.
    static void checkAndHandleRollback(uint8_t maxAttempts = 3);

    // Call once the running image has proven itself (e.g. after the
    // sketch's own post-update self-check passes). Marks this slot
    // confirmed in NVS, resets its boot-attempt counter, and calls
    // esp_ota_mark_app_valid_cancel_rollback() (a harmless no-op if the
    // bootloader wasn't built with native rollback support).
    static void confirmValid();

    // Label of the currently-running partition ("factory", "ota_0", "ota_1").
    static std::string getRunningSlotLabel();

    // How many consecutive un-confirmed boots the running slot has seen,
    // including this one. 0 if the running slot is already confirmed.
    static uint8_t getBootAttemptCount();

    // Whether the running slot is currently marked confirmed in NVS. Reads
    // fresh from NVS each call (cheap; not cached), so it's accurate even if
    // checkAndHandleRollback() hasn't run yet in this boot.
    static bool isRunningSlotConfirmed();

    // Optional: if set, checkAndHandleRollback() reads this GPIO (with its
    // internal pull-up enabled) before doing anything else, and forces a
    // boot into `factory` -- regardless of rollback state -- if it reads
    // LOW (a button wired to GND, the standard boot-button convention on
    // ESP32 devkits). Pass -1 (the default) to disable this check.
    static void setRecoveryButtonPin(int8_t gpio);

private:
    static int8_t recoveryButtonPin;
};

#endif
