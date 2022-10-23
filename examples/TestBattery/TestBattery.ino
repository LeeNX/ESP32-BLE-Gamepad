
/*
 * A simple sketch that maps a single pin on the ESP32 to a single button on the controller
 */

#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif

#define CORE_DEBUG_LEVEL 3
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

//#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
//#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
//#define SERVICE_UUID_DEVICE_INFORMATION "180A" 
//#define CHARACTERISTIC_UUID_SERIAL_NUMBER "2A25" // Serial Number String - 0x2A25

#include <Arduino.h>
#include <BleGamepad.h> // https://github.com/lemmingDev/ESP32-BLE-Gamepad

#define BUTTONPIN 20 // Pin button is attached to

#define numOfButtons 64
#define numOfHatSwitches 4

int batteryLevel = 75;
signed int batteryLevelDirection = 1;

// Shows how you can customise the device name, manufacturer name and initial battery level
BleGamepad bleGamepad("ESP32TestOrg", "ESP32GamePads", batteryLevel);
BleGamepadConfiguration bleGamepadConfig;

// Use the procedure below to set a custom Bluetooth MAC address
// Compiler adds 0x02 to the last value of board's base MAC address to get the BT MAC address, so take 0x02 away from the value you actually want when setting
// I've noticed the first number is a little picky and if set incorrectly don't work and will default to the board's embedded address
// 0xAA definately works, so use that, or experiment
//uint8_t newMACAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF - 0x02};

int previousButton1State = HIGH;
int previousButton2State = HIGH;

// constants won't change. Used here to set a pin number:
//const int ledPin =  LED_BUILTIN;// the number of the LED pin
const int ledPin =  2;// the number of the LED pin

// Variables will change:
int ledState = LOW;             // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change:
//const long interval = 1000;           // interval at which to blink (milliseconds)
long interval = 1000;

void setup()
{
  // set the digital pin as output:
    
    Serial.begin(115200);
    Serial.println("Starting BLE work!");
  
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, ledState);

    pinMode(BUTTONPIN, INPUT_PULLUP);

    Serial.println("Starting Game work!");
    Serial.println(millis());
    
    bleGamepadConfig.setAutoReport(true);
    //bleGamepadConfig.setAutoReport(false); // Disable auto reports --> You then need to force HID updates with bleGamepad.sendReport()
    bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
    bleGamepadConfig.setButtonCount(numOfButtons);
    bleGamepadConfig.setHatSwitchCount(numOfHatSwitches);

    bleGamepadConfig.setVid(0xe502);
    bleGamepadConfig.setPid(0xabcd);

    bleGamepadConfig.setModelNumber("1.0");
    bleGamepadConfig.setSoftwareRevision("Software Rev 1");
    bleGamepadConfig.setSerialNumber("9876543210");
    bleGamepadConfig.setFirmwareRevision("2.0");
    bleGamepadConfig.setHardwareRevision("1.7");

    // Some non-Windows operating systems and web based gamepad testers don't like min axis set below 0, so 0 is set by default
    //bleGamepadConfig.setAxesMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    bleGamepadConfig.setAxesMin(0x0000); // 0 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    bleGamepadConfig.setAxesMax(0x7FFF); // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal 
    bleGamepad.begin(&bleGamepadConfig); // Simulation controls, special buttons and hats 2/3/4 are disabled by default

    //bleGamepad.begin();

   // changing bleGamepadConfig after the begin function has no effect, unless you call the begin function again

    //esp_base_mac_addr_set(&newMACAddress[0]); // Set new MAC address

    Serial.println(millis());

    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, ledState);

    Serial.println("Running ...");

}

void loop()
{
    // here is where you'd put code that needs to be running all the time.

  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    Serial.println("unblink");
    Serial.println(currentMillis);
    Serial.println(bleGamepad.isConnected());
    Serial.println("blink");

/*
    if (batteryLevel < 30) {
      batteryLevel++;
      Serial.print("battery level: up");
    }

    if (batteryLevel > 80) {
      batteryLevel--;
      Serial.print("battery level: down");
    }
*/

    if (batteryLevel <= 15) {
      Serial.print("battery level: low switch");
      //batteryLevel = 20;
      batteryLevelDirection = +1;
    }

    if (batteryLevel >= 95) {
      Serial.print("battery level: high switch");
      //batteryLevel = 90;
      batteryLevelDirection = -1;
    }

    batteryLevel += batteryLevelDirection;

    Serial.print("battery level: ");
    Serial.println(batteryLevel);
    bleGamepad.setBatteryLevel(batteryLevel);


    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
  
    if (bleGamepad.isConnected())
    {
      //  Serial.print("ControllerType: ");
      //  Serial.println(bleGamepadConfig.getControllerType());
      interval = 2500;

        // Button 1
        int currentButton1State = digitalRead(BUTTONPIN);

        if (currentButton1State != previousButton1State)
        {
            if (currentButton1State == LOW)
            {
                bleGamepad.press(BUTTON_1);
            }
            else
            {
                bleGamepad.release(BUTTON_1);
            }
        }
        previousButton1State = currentButton1State;


        // Button 2
        int currentButton2State = ledState;

        if (currentButton2State != previousButton2State)
        {
            if (currentButton2State == LOW)
            {
                bleGamepad.press(BUTTON_2);
            }
            else
            {
                bleGamepad.release(BUTTON_2);
            }
        }
        previousButton2State = currentButton2State;

    } else {
        interval = 500;
    }
}
