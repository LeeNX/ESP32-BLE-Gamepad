#ifndef ESP32_BLE_GAMEPAD_OUTPUT_CALLBACKS_H
#define ESP32_BLE_GAMEPAD_OUTPUT_CALLBACKS_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <BLEServer.h>
#include "BLE2902.h"
#include "BLECharacteristic.h"

// key report back
typedef struct{
        uint8_t bmPlayerLED1 : 1;
        uint8_t bmPlayerLED2 : 1;
        uint8_t bmPlayerLED3 : 1;
        uint8_t bmPlayerLED4 : 1;
        uint8_t bmReserved : 4;
} PlayerLeds;
using callBackFunc = void (*)(PlayerLeds*);

class GamepadOutputCallbacks : public BLECharacteristicCallbacks
{
public:
  callBackFunc func = [](PlayerLeds*){ };
  GamepadOutputCallbacks(void);
  void onWrite(BLECharacteristic* me);
};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_GAMEPAD_OUTPUT_CALLBACKS_H
