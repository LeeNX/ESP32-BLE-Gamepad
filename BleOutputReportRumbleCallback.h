#pragma once

#include <NimBLECharacteristic.h>
#include "esp_log.h"

class BleGamepadOutputReportRumbleCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() >= 2) {
      uint8_t weak = value[0];
      uint8_t strong = value[1];
      ESP_LOGI("RUMBLE", "Rumble command received: weak=%d, strong=%d", weak, strong);
      // TODO: Add GPIO or motor control logic here
    } else {
      ESP_LOGW("RUMBLE", "Invalid rumble report length: %d", value.length());
    }
  }
};
