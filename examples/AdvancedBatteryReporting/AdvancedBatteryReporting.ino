/*
   This example builds on SetBatteryLevel and SetBatteryPowerState by showing the three newer,
   fully OPT-IN ways to expose battery/power information. None of these are enabled by default,
   so existing sketches that don't call these functions are completely unaffected.

   1) bleGamepadConfig.setIncludeBatteryLevelInReport(true)
      Adds a standard HID "Battery Strength" field (0-100) to the gamepad's own HID input report,
      in addition to the existing GATT Battery Level characteristic. Some hosts (e.g. Linux) turn
      this into a system-level battery indicator automatically, without needing to read GATT.

   2) bleGamepadConfig.setIncludePowerStateInReport(true)
      Adds a single vendor-defined byte to the HID input report with a simplified power state:
        GAMEPAD_POWER_STATE_UNKNOWN    (0)
        GAMEPAD_POWER_STATE_CHARGING   (1)
        GAMEPAD_POWER_STATE_ON_BATTERY (2)
        GAMEPAD_POWER_STATE_FULL       (3)
      Set with bleGamepad.setPowerState(...). This is separate from the more detailed
      setPowerStateAll()/setBatteryPowerInformation() GATT characteristic used in SetBatteryPowerState.

   3) bleGamepadConfig.setEnableManufacturerData(true)
      Broadcasts battery level + power state in the BLE advertising packet's Manufacturer Specific
      Data, so a passive BLE scanner (e.g. nRF Connect) can read them WITHOUT connecting. Like any
      classic (non-extended) BLE advertising, this only broadcasts while the device isn't connected
      to a client - it resumes automatically on disconnect.
      Optionally set bleGamepadConfig.setManufacturerCompanyId(...) - defaults to 0xFFFF, the
      Bluetooth SIG's reserved "for testing" company ID.

      The broadcast payload is 3 bytes: [companyId low, companyId high, packedState], where packedState
      bit-packs power state and battery into a single byte to keep the advertising packet as small as
      possible - bits 7-6 = power state (0-3), bits 5-1 = battery level scaled from 0-100 down to 0-31
      (~3.2% steps), bit 0 reserved.

      Note the updates below run every loop regardless of connection state - that's the point of
      broadcasting via advertising: a scanner can see it change WITHOUT ever connecting.

   4) bleGamepadConfig.setManufacturerDataUpdateInterval(ms)
      Each live update is a real over-the-air/HCI operation, so it's throttled separately from how
      often setBatteryLevel()/setPowerState() are called - defaults to 5000ms. Calls made faster than
      that just update the GATT/HID values immediately and get picked up by the next allowed
      broadcast; nothing is lost, the radio update is just debounced. Set to 0 to disable throttling.
*/

#include <Arduino.h>
#include <BleGamepad.h>

BleGamepad bleGamepad("Advanced Battery Gamepad", "lemmingDev", 100);
BleGamepadConfiguration bleGamepadConfig;

int batteryLevel = 100;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  bleGamepadConfig.setIncludeBatteryLevelInReport(true);
  bleGamepadConfig.setIncludePowerStateInReport(true);

  bleGamepadConfig.setEnableManufacturerData(true);
  bleGamepadConfig.setManufacturerCompanyId(0xFFFF); // Defaults to 0xFFFF if not set

  bleGamepad.begin(&bleGamepadConfig);
}

void loop()
{
  // Runs regardless of connection state - see the note above on why.
  if (batteryLevel > 0)
  {
    batteryLevel -= 1;
  }

  Serial.print("Battery Level Set To: ");
  Serial.println(batteryLevel);
  bleGamepad.setBatteryLevel(batteryLevel); // Also updates the HID report field and manufacturer data

  if (batteryLevel > 20)
  {
    bleGamepad.setPowerState(GAMEPAD_POWER_STATE_ON_BATTERY);
  }
  else
  {
    bleGamepad.setPowerState(GAMEPAD_POWER_STATE_CHARGING); // Pretend it got plugged in when low
  }

  delay(10000);
}
