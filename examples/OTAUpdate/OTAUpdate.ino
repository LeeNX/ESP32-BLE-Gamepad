/*
 * A gamepad with the BLE OTA update service enabled -- push a new firmware
 * image over Web Bluetooth (see ../../tools/ota-web/index.html) or any GATT
 * client that speaks the protocol in ../../BleOTA.md, and it flashes into
 * whichever of ota_0/ota_1 isn't currently running.
 *
 * Needs the custom partition table in platformio.ini (board_build.partitions
 * = ../ota/partitions.csv) -- factory + ota_0 + ota_1 + otadata. Flashing
 * this sketch the normal way (pio run -t upload) writes it into whichever
 * app slot the bootloader currently considers default; from then on, OTA
 * updates alternate between ota_0/ota_1 and this initial copy stops being
 * relevant (it isn't `factory` unless you deliberately flash it there --
 * see ../OTARecovery for what should actually live in `factory`).
 *
 * Rollback: BleOTARollback::checkAndHandleRollback() runs before anything
 * else in setup(). If a freshly-flashed image never calls confirmValid()
 * within SELF_CHECK_WINDOW_MS across enough reboots, it gets rolled back
 * automatically. This sketch's "self-check" is just staying up and BLE-
 * connectable for a while -- a real one should exercise whatever this
 * device actually does.
 */

#include <Arduino.h>
#include <BleGamepad.h> // https://github.com/lemmingDev/ESP32-BLE-Gamepad

#define SELF_CHECK_WINDOW_MS 10000 // how long a freshly-flashed image must run before confirmValid()

BleGamepad bleGamepad("ESP32 BLE Gamepad OTA", "Espressif", 100, /*delayAdvertising=*/true);

bool selfCheckPassed = false;

void setup()
{
    // Must be the very first thing that runs -- before Serial.begin(), before
    // constructing anything BLE-related, before anything that could crash.
    BleOTARollback::checkAndHandleRollback();

    Serial.begin(115200);
    Serial.printf("[OTAUpdate] Running from '%s', boot attempt %u\n",
                   BleOTARollback::getRunningSlotLabel().c_str(), BleOTARollback::getBootAttemptCount());

    BleGamepadConfiguration bleGamepadConfig;
    bleGamepad.begin(&bleGamepadConfig);
    bleGamepad.beginOTA(); // safe to call even if this image never gets confirmed -- it's just a GATT service

    Serial.println("[OTAUpdate] Ready. Waiting " + String(SELF_CHECK_WINDOW_MS) + "ms before confirming this image valid.");
}

void loop()
{
    // Stand-in "self-check": if this were a real device, this window would
    // be where you'd verify whatever actually matters (sensors respond,
    // outputs work, no crash-looping) before trusting the new image.
    if (!selfCheckPassed && millis() >= SELF_CHECK_WINDOW_MS)
    {
        selfCheckPassed = true;
        BleOTARollback::confirmValid();
        Serial.println("[OTAUpdate] Self-check window elapsed -- image confirmed valid.");
    }
}
