/*
   Demonstrates putting the ESP32 into deep sleep to save battery when the gamepad has been sitting
   disconnected and untouched for a while, waking back up on a physical button press.

   Wiring: a momentary button between GPIO4 and GND (INPUT_PULLUP, active-low). GPIO4 was picked
   because it's RTC-capable with a real internal pull-up on classic ESP32, and is a plain usable
   GPIO on ESP32-C3 - change WAKE_BUTTON_PIN below if that pin isn't free on your board.

   The two target chips need genuinely different APIs to configure a GPIO deep-sleep wake source
   (confirmed against the installed framework headers, not just docs):
     - Classic ESP32 (e.g. nodemcu-32s): esp_sleep_enable_ext0_wakeup() - the only chip with ext0.
     - ESP32-C3 (e.g. lolin_c3_mini): has no ext0/ext1 at all; needs the newer unified
       esp_deep_sleep_enable_gpio_wakeup() instead.

   Deep sleep wake is a full chip reset - setup() runs again from scratch exactly like a cold boot,
   there's no state to restore, and the BLE stack/advertising is completely re-initialized.

   IMPORTANT tradeoff: deep sleep cuts the radio entirely, unlike a normal disconnect (where
   advertising - including the battery/power manufacturer data broadcast - keeps running). While
   asleep, a scanner or charger CANNOT read battery/charge status at all; you have to wake the
   device (button press) before it's visible again. That's the cost of the biggest power saving
   mode - if you need remote status checks even while idle, don't deep sleep, just leave it
   advertising (see AdvancedBatteryReporting).
*/

#include <Arduino.h>
#include <BleGamepad.h>
#include <esp_sleep.h>

#define WAKE_BUTTON_PIN 4
#define IDLE_TIMEOUT_MS 30000 // Artificially short for demo purposes - a real product would use minutes

BleGamepad bleGamepad("Sleepy Pad");

unsigned long lastActivityTime = 0;
bool buttonWasPressed = false;

void printWakeReason()
{
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
    if (wakeupReason == ESP_SLEEP_WAKEUP_EXT0 || wakeupReason == ESP_SLEEP_WAKEUP_GPIO)
    {
        Serial.println("[DeepSleepIdle] Woke up from deep sleep via button press.");
    }
    else
    {
        Serial.println("[DeepSleepIdle] Fresh boot (power-on or reset), not a sleep wake.");
    }
}

void goToSleep()
{
    Serial.println("[DeepSleepIdle] Idle timeout reached, going to deep sleep. Press the button to wake.");
    Serial.flush();

#if CONFIG_IDF_TARGET_ESP32
    esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_BUTTON_PIN, 0); // Wake when the pin reads LOW
#elif CONFIG_IDF_TARGET_ESP32C3
    esp_deep_sleep_enable_gpio_wakeup(BIT64(WAKE_BUTTON_PIN), ESP_GPIO_WAKEUP_GPIO_LOW);
#else
#error "DeepSleepIdle only implements GPIO wakeup for classic ESP32 and ESP32-C3 - add a case for your target."
#endif

    esp_deep_sleep_start(); // Never returns - execution resumes at setup() on wake
}

void setup()
{
    Serial.begin(115200);
    pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);

    printWakeReason();

    BleGamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setEnableManufacturerData(true); // So charge status is visible while awake, without connecting

    bleGamepad.begin(&bleGamepadConfig);
    bleGamepad.setBatteryLevel(100);
    bleGamepad.setPowerState(GAMEPAD_POWER_STATE_ON_BATTERY);

    lastActivityTime = millis();

    Serial.println("[DeepSleepIdle] Ready. Will sleep after " + String(IDLE_TIMEOUT_MS / 1000) + "s idle (disconnected + no button presses).");
}

void loop()
{
    bool pressed = (digitalRead(WAKE_BUTTON_PIN) == LOW);

    if (pressed && !buttonWasPressed)
    {
        bleGamepad.press(BUTTON_1);
        lastActivityTime = millis(); // A press counts as activity, resetting the idle clock
    }
    else if (!pressed && buttonWasPressed)
    {
        bleGamepad.release(BUTTON_1);
    }
    buttonWasPressed = pressed;

    // Never sleep mid-session - a connected client counts as permanent activity
    if (bleGamepad.isConnected())
    {
        lastActivityTime = millis();
    }

    if (millis() - lastActivityTime >= IDLE_TIMEOUT_MS)
    {
        goToSleep();
    }
}
