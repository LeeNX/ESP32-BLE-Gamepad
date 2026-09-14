/*
 * Minimal factory-recovery image: no BleGamepad, no HID, nothing but the
 * BLE OTA service (see ../../BleOTA.md). This is what should live in the
 * `factory` partition -- flashed there once, and never touched by OTA
 * again (esp_ota_get_next_update_partition() only ever returns ota_0/
 * ota_1). If both OTA slots are ever exhausted (rollback ran out of
 * attempts on both, or a bad image bricked both), this is the last resort:
 * a device that can still be reached over BLE and re-flashed with a good
 * image into ota_0.
 *
 * Deliberately doesn't call BleOTARollback::checkAndHandleRollback() --
 * keeping this image's dependency surface as small as possible matters
 * more here than rollback bookkeeping for a slot that's never overwritten.
 *
 * Flashing this into `factory` specifically needs a direct esptool.py
 * invocation, since `pio run -t upload` always targets the currently
 * selected default app offset, not an arbitrary named slot:
 *
 *   pio run -e esp32dev
 *   esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x10000 \
 *       .pio/build/esp32dev/firmware.bin
 *
 * (0x10000 is `factory`'s offset in ../ota/partitions.csv -- adjust if you
 * change that file.)
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <BleOTA.h>

#define RECOVERY_DEVICE_NAME "ESP32 BLE Gamepad (Recovery)"

BleOTA *ota = nullptr;

void setup()
{
    Serial.begin(115200);
    Serial.println("[OTARecovery] Starting -- BLE OTA service only, no gamepad HID.");

    NimBLEDevice::init(RECOVERY_DEVICE_NAME);
    NimBLEDevice::setSecurityAuth(true, false, false); // bonding, no MITM, no SC -- required for BleOTA's WRITE_ENC characteristics

    NimBLEServer *pServer = NimBLEDevice::createServer();
    ota = new BleOTA(pServer);
    ota->begin(); // creates the OTA service and starts advertising (stop/reconfigure/start, same as BleNUS)

    Serial.println("[OTARecovery] Advertising. Connect and bond, then push a firmware image over the OTA service.");
}

void loop()
{
    delay(1000);
}
