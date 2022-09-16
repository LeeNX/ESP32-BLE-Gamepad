#include "BleGamepadOutputCallbacks.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG "BLEGamepad"
#else
  #include "esp_log.h"
  static const char *LOG_TAG = "BLEGamepad";
#endif

//ESP_LOGI(LOG_TAG, "Callbacks file included");

BleGamepadOutputCallbacks::BleGamepadOutputCallbacks(void) {
  ESP_LOGI(LOG_TAG, "Callbacks initialised");
}

void BleGamepadOutputCallbacks::onWrite(NimBLECharacteristic* me) {
  uint8_t* value = (uint8_t*)(me->getValue().c_str());
  ESP_LOGI(LOG_TAG, "leds: %d", *value);
  ESP_LOGI(LOG_TAG, "Callbacks written to");

  //Serial.print("LED state: ");
  //Serial.print((int) *value);
  //Serial.println();
}
