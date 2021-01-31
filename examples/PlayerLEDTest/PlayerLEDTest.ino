/*
 * A simple sketch that maps a single pin on the ESP32 to a single button on the controller
 */

#include <BleGamepad.h>   // https://github.com/lemmingDev/ESP32-BLE-Gamepad

BleGamepad bleGamepad;

int previousButton1State = HIGH;

int PlayerLED1 = 0;
int PlayerLED2 = 0;
int PlayerLED3 = 0;
int PlayerLED4 = 0;

//
// Register your callback function to be notified of LED changes
// This is called from the bluetooth stack, so don't do system calls
// from within this function.
//
void GamePadLedCb(PlayerLeds *gpls)
{
  // digitalWrite(2,gpls->bmPlayerLED1);
  // digitalWrite(2,gpls->bmPlayerLED2);
  // ...
  PlayerLED1 = gpls->bmPlayerLED1;
  PlayerLED2 = gpls->bmPlayerLED2;
  PlayerLED3 = gpls->bmPlayerLED3;
  PlayerLED4 = gpls->bmPlayerLED4;

}

void setup() 
{
  Serial.begin(115200);
  pinMode(2, INPUT_PULLUP);
  Serial.println("Starting BLE work!");
  bleGamepad.begin();
  delay(1000);  //must have delay for the BLE  finish inital
  bleGamepad.setLedChangeCallBack(GamePadLedCb);

}

void loop() 
{
  if(bleGamepad.isConnected()) 
  {

    Serial.println(PlayerLED1);

    int currentButton1State = digitalRead(2);

    if (currentButton1State != previousButton1State)
    {
      if(currentButton1State == LOW)
      {
        bleGamepad.press(BUTTON_1);
      }
      else
      {
        bleGamepad.release(BUTTON_1);
      }
    }
    previousButton1State = currentButton1State;
  }

  Serial.println("Waiting 5 seconds...");
  delay(5000);

}
