#include "GamepadOutputCallbacks.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif

//ESP_LOGI(LOG_TAG, "Callbacks file included");

GamepadOutputCallbacks::GamepadOutputCallbacks(void) {
  ESP_LOGI(LOG_TAG, "Callbacks initialised");
}

void GamepadOutputCallbacks::onWrite(BLECharacteristic* me) {
//  uint8_t* value = (uint8_t*)(me->getValue().c_str());
  PlayerLeds* gpled = (PlayerLeds*)(me->getValue().c_str());
  ESP_LOGI(LOG_TAG, "leds: %d", *gpled);
  ESP_LOGI(LOG_TAG, "Callbacks written to");

  //Serial.print("LED state: ");
  //Serial.print((int) *value);
  //Serial.println();

  // if(func!=NULL)
  func(gpled);
}
