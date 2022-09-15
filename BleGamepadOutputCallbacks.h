#ifndef ESP32_BLE_GAMEPAD_OUTPUT_CALLBACKS_H
#define ESP32_BLE_GAMEPAD_OUTPUT_CALLBACKS_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimconfig.h"
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include <NimBLEServer.h>
#include "NimBLECharacteristic.h"
//#include "NimBLEHIDDevice.h"

//class GamepadOutputCallbacks : public BLECharacteristicCallbacks
class BleGamepadOutputCallbacks : public NimBLECharacteristicCallbacks
{
public:
  BleGamepadOutputCallbacks(void);
  //void onWrite(BLECharacteristic* me);
  void onWrite(NimBLECharacteristic* me);
};

#endif // CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_CONNECTION_STATUS_H
