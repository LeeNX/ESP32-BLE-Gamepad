## POSSIBLE BREAKING CHANGES - PLEASE READ
A large code rebase (configuration class) along with some extra features (start, select, menu, home, back, volume up, volume down and volume mute buttons) has been committed thanks to @dexterdy

Since version 5 of this library, the axes and simulation controls have configurable min and max values
The decision was made to set defaults to 0 for minimum and 32767 for maximum (previously -32767 to 32767)
This was due to the fact that non-Windows operating systems and some online web-based game controller testers didn't play well with negative numbers. Existing sketches should take note, and see the DrivingControllerTest example for how to set back to -32767 if wanted

This version of the library has been tested against NimBLE-Arduino version 1.4; the latest released version --> https://github.com/h2zero/NimBLE-Arduino/releases/tag/1.4.0

Please see updated examples

## NimBLE
Since version 3 of this library, the more efficient NimBLE library is used instead of the default BLE implementation
Please use the library manager to install it, or get it from here: https://github.com/h2zero/NimBLE-Arduino
Since version 3, this library also supports a configurable HID desciptor, which allows users to customise how the device presents itself to the OS (number of buttons, hats, axes, sliders, simulation controls etc).
See the examples for guidance.

# ESP32-BLE-Gamepad

## License
Published under the MIT license. Please see license.txt.

It would be great however if any improvements are fed back into this version.

## Features

 - [x] Button press (128 buttons)
 - [x] Button release (128 buttons)
 - [x] Axes movement (6 axes (configurable resolution up to 16 bit) (x, y, z, rZ, rX, rY) --> (Left Thumb X, Left Thumb Y, Right Thumb X, Right Thumb Y, Left Trigger, Right Trigger))
 - [x] 2 Sliders (configurable resolution up to 16 bit) (Slider 1 and Slider 2)
 - [x] 4 point of view hats (ie. d-pad plus 3 other hat switches)
 - [x] Simulation controls (rudder, throttle, accelerator, brake, steering)
 - [x] Special buttons (start, select, menu, home, back, volume up, volume down, volume mute) all disabled by default
 - [x] Configurable HID descriptor
 - [x] Configurable VID and PID values
 - [x] Configurable BLE characteristics (name, manufacturer, model number, software revision, serial number, firmware revision, hardware revision)
 - [ ] Report optional battery level to host <-- currently disabled <-- awaiting NimBLE support
 - [x] Uses efficient NimBLE bluetooth library
 - [x] Compatible with Windows
 - [x] Compatible with Android (Android OS maps default buttons / axes / hats slightly differently than Windows)
 - [x] Compatible with Linux (limited testing)
 - [x] Compatible with MacOS X (limited testing)
 - [ ] Compatible with iOS (No - not even for accessibility switch - This is not a “Made for iPhone” (MFI) compatible device)

## Installation
- (Make sure you can use the ESP32 with the Arduino IDE. [Instructions can be found here.](https://github.com/espressif/arduino-esp32#installation-instructions))
- [Download the latest release of this library from the release page.](https://github.com/lemmingDev/ESP32-BLE-Gamepad/releases)
- In the Arduino IDE go to "Sketch" -> "Include Library" -> "Add .ZIP Library..." and select the file you just downloaded.
- In the Arduino IDE go to "Tools" -> "Manage Libraries..." -> Filter for "NimBLE-Arduino" by h2zero and install.
- You can now go to "File" -> "Examples" -> "ESP32 BLE Gamepad" and select an example to get started.

## Example

``` C++
/*
 * This example turns the ESP32 into a Bluetooth LE gamepad that presses buttons and moves axis
 *
 * At the moment we are using the default settings, but they can be canged using a BleGamepadConfig instance as parameter for the begin function.
 *
 * Possible buttons are:
 * BUTTON_1 through to BUTTON_16
 * (16 buttons by default. Library can be configured to use up to 128)
 *
 * Possible DPAD/HAT switch position values are:
 * DPAD_CENTERED, DPAD_UP, DPAD_UP_RIGHT, DPAD_RIGHT, DPAD_DOWN_RIGHT, DPAD_DOWN, DPAD_DOWN_LEFT, DPAD_LEFT, DPAD_UP_LEFT
 * (or HAT_CENTERED, HAT_UP etc)
 *
 * bleGamepad.setAxes sets all axes at once. There are a few:
 * (x axis, y axis, z axis, rx axis, ry axis, rz axis, slider 1, slider 2)
 *
 * Library can also be configured to support up to 5 simulation controls
 * (rudder, throttle, accelerator, brake, steering), but they are not enabled by default.
 *
 * Library can also be configured to support different function buttons
 * (start, select, menu, home, back, volume increase, volume decrease, volume mute)
 * start and select are enabled by default
 */

#include <Arduino.h>
#include <BleGamepad.h>

BleGamepad bleGamepad;

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");
    bleGamepad.begin();
    // The default bleGamepad.begin() above enables 16 buttons, all axes, one hat, and no simulation controls or special buttons
}

void loop()
{
    if (bleGamepad.isConnected())
    {
        Serial.println("Press buttons 5, 16 and start. Move all enabled axes to max. Set DPAD (hat 1) to down right.");
        bleGamepad.press(BUTTON_5);
        bleGamepad.press(BUTTON_16);
        bleGamepad.pressStart();
        bleGamepad.setAxes(32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767);
        bleGamepad.setHat1(HAT_DOWN_RIGHT);
        // All axes, sliders, hats etc can also be set independently. See the IndividualAxes.ino example
        delay(500);

        Serial.println("Release button 5 and start. Move all axes to min. Set DPAD (hat 1) to centred.");
        bleGamepad.release(BUTTON_5);
        bleGamepad.releaseStart();
        bleGamepad.setHat1(HAT_CENTERED);
        bleGamepad.setAxes(0, 0, 0, 0, 0, 0, 0, 0);
        delay(500);
    }
}
```
By default, reports are sent on every button press/release or axis/slider/hat/simulation movement, however this can be disabled, and then you manually call sendReport on the gamepad instance as shown in the IndividualAxes.ino example.

VID and PID values can be set. See TestAll.ino for example.

There is also Bluetooth specific information that you can use (optional):

Instead of `BleGamepad bleGamepad;` you can do `BleGamepad bleGamepad("Bluetooth Device Name", "Bluetooth Device Manufacturer", 100);`.
The third parameter is the initial battery level of your device.
By default the battery level will be set to 100%, the device name will be `ESP32 BLE Gamepad` and the manufacturer will be `Espressif`.


## Credits
Credits to [T-vK](https://github.com/T-vK) as this library is based on his ESP32-BLE-Mouse library (https://github.com/T-vK/ESP32-BLE-Mouse) that he provided.

Credits to [chegewara](https://github.com/chegewara) as the ESP32-BLE-Mouse library is based on [this piece of code](https://github.com/nkolban/esp32-snippets/issues/230#issuecomment-473135679) that he provided.

Credits to [wakwak-koba](https://github.com/wakwak-koba) for the NimBLE [code](https://github.com/wakwak-koba/ESP32-NimBLE-Gamepad) that he provided.

## Notes
This library allows you to make the ESP32 act as a Bluetooth Gamepad and control what it does.  
Relies on [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)

Use [this](http://www.planetpointy.co.uk/joystick-test-application/) Windows test app to test/see all of the buttons

You might also be interested in:
- [ESP32-BLE-Mouse](https://github.com/T-vK/ESP32-BLE-Mouse)
- [ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard)

or the NimBLE versions at

- [ESP32-NimBLE-Mouse](https://github.com/wakwak-koba/ESP32-NimBLE-Mouse)
- [ESP32-NimBLE-Keyboard](https://github.com/wakwak-koba/ESP32-NimBLE-Keyboard)

## Troubleshooting
Raspberry Pi command line pairing and Troubleshooting
```bash
sudo hciconfig hci0 up
sudo hcitool lescan
sudo btmon

sudo bluetoothctl
  agent on
  default-agent
  pairable on
  discoverable on

sudo bluetoothctl scan on
```
Find the mac address for your device
```bash
sudo bluetoothctl trust 34:94:54:35:1D:F6
```
Then the next two command need to be run immediately after one another, else pairing might not be successful
```bash
sudo bluetoothctl pair 34:94:54:35:1D:F6 && \
sudo bluetoothctl connect 34:94:54:35:1D:F6
```
Output would look a little like the following
```
[CHG] Device 34:94:54:35:1D:F6 Connected: yes
[CHG] Device 34:94:54:35:1D:F6 UUIDs: 00001800-0000-1000-8000-00805f9b34fb
[CHG] Device 34:94:54:35:1D:F6 UUIDs: 00001801-0000-1000-8000-00805f9b34fb
[CHG] Device 34:94:54:35:1D:F6 UUIDs: 0000180a-0000-1000-8000-00805f9b34fb
[CHG] Device 34:94:54:35:1D:F6 UUIDs: 0000180f-0000-1000-8000-00805f9b34fb
[CHG] Device 34:94:54:35:1D:F6 UUIDs: 00001812-0000-1000-8000-00805f9b34fb
[CHG] Device 34:94:54:35:1D:F6 ServicesResolved: yes
[CHG] Device 34:94:54:35:1D:F6 Paired: yes
[NEW] Primary Service (Handle 0x7d44)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service0006
	00001801-0000-1000-8000-00805f9b34fb
	Generic Attribute Profile
[NEW] Characteristic (Handle 0x2f48)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service0006/char0007
	00002a05-0000-1000-8000-00805f9b34fb
	Service Changed
[NEW] Descriptor (Handle 0x2f90)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service0006/char0007/desc0009
	00002902-0000-1000-8000-00805f9b34fb
	Client Characteristic Configuration
[NEW] Primary Service (Handle 0x8b74)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a
	0000180a-0000-1000-8000-00805f9b34fb
	Device Information
[NEW] Characteristic (Handle 0x2f48)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char000b
	00002a50-0000-1000-8000-00805f9b34fb
	PnP ID
[NEW] Characteristic (Handle 0x2f48)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char000d
	00002a29-0000-1000-8000-00805f9b34fb
	Manufacturer Name String
[NEW] Characteristic (Handle 0x2f48)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char000f
	00002a24-0000-1000-8000-00805f9b34fb
	Model Number String
[NEW] Characteristic (Handle 0x2f48)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char0011
	00002a28-0000-1000-8000-00805f9b34fb
	Software Revision String
[NEW] Characteristic (Handle 0x2f48)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char0013
	00002a25-0000-1000-8000-00805f9b34fb
	Serial Number String
[NEW] Characteristic (Handle 0x2f48)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char0015
	00002a26-0000-1000-8000-00805f9b34fb
	Firmware Revision String
[NEW] Characteristic (Handle 0x2f48)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char0017
	00002a27-0000-1000-8000-00805f9b34fb
	Hardware Revision String
[NEW] Primary Service (Handle 0xa364)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service0026
	0000180f-0000-1000-8000-00805f9b34fb
	Battery Service
[NEW] Characteristic (Handle 0x2f48)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service0026/char0027
	00002a19-0000-1000-8000-00805f9b34fb
	Battery Level
[NEW] Descriptor (Handle 0x2f90)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service0026/char0027/desc0029
	00002902-0000-1000-8000-00805f9b34fb
	Client Characteristic Configuration
[NEW] Descriptor (Handle 0x2f90)
	/org/bluez/hci0/dev_34_94_54_35_1D_F6/service0026/char0027/desc002a
	00002904-0000-1000-8000-00805f9b34fb
	Characteristic Format
Pairing successful
Attempting to connect to 34:94:54:35:1D:F6
Connection successful
```
Found some times the layout might changes, get another list
```
sudo bluetoothctl
menu gatt
list-attributes 34:94:54:35:1D:F6
```
Output?
```
[ESP32TestOrg]# list-attributes 34:94:54:35:1D:F6
Primary Service (Handle 0x5f9c)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029
    0000180f-0000-1000-8000-00805f9b34fb
    Battery Service
Characteristic (Handle 0x8900)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a
    00002a19-0000-1000-8000-00805f9b34fb
    Battery Level
Descriptor (Handle 0x0000)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a/desc002d
    00002904-0000-1000-8000-00805f9b34fb
    Characteristic Format
Descriptor (Handle 0x0000)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a/desc002c
    00002902-0000-1000-8000-00805f9b34fb
    Client Characteristic Configuration
Primary Service (Handle 0x819c)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a
    0000180a-0000-1000-8000-00805f9b34fb
    Device Information
Characteristic (Handle 0xd400)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char0017
    00002a27-0000-1000-8000-00805f9b34fb
    Hardware Revision String
Characteristic (Handle 0xd400)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char0015
    00002a26-0000-1000-8000-00805f9b34fb
    Firmware Revision String
Characteristic (Handle 0xd400)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char0013
    00002a25-0000-1000-8000-00805f9b34fb
    Serial Number String
Characteristic (Handle 0xd400)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char0011
    00002a28-0000-1000-8000-00805f9b34fb
    Software Revision String
Characteristic (Handle 0xd400)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char000f
    00002a24-0000-1000-8000-00805f9b34fb
    Model Number String
Characteristic (Handle 0xd400)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char000d
    00002a29-0000-1000-8000-00805f9b34fb
    Manufacturer Name String
Characteristic (Handle 0xd400)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char000b
    00002a50-0000-1000-8000-00805f9b34fb
    PnP ID
Primary Service (Handle 0x819c)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0006
    00001801-0000-1000-8000-00805f9b34fb
    Generic Attribute Profile
Characteristic (Handle 0xd400)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0006/char0007
    00002a05-0000-1000-8000-00805f9b34fb
    Service Changed
Descriptor (Handle 0x0000)
    /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0006/char0007/desc0009
    00002902-0000-1000-8000-00805f9b34fb
    Client Characteristic Configuration
```
```
dbus-send --print-reply=literal --system --dest=org.bluez \
  /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a \
  org.freedesktop.DBus.Properties.GetAll string:"org.bluez.GattCharacteristic1"
```
```
dbus-send --print-reply=literal --system --dest=org.bluez \
  /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a \
  org.freedesktop.DBus.Properties.Get \
  string:"org.bluez.GattCharacteristic1" string:"Value"
```
```
dbus-send --print-reply=literal --system --dest=org.bluez \
  /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a/desc002d \
  org.freedesktop.DBus.Properties.GetAll \
  string:"org.bluez.GattDescriptor1"
dbus-send --print-reply=literal --system --dest=org.bluez \
  /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a/desc002d \
  org.freedesktop.DBus.Properties.Get \
  string:"org.bluez.GattDescriptor1" \
  string:"Value"
dbus-send --print-reply=literal --system --dest=org.bluez \
  /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a \
  org.freedesktop.DBus.Properties.GetAll \
  string:"org.bluez.GattCharacteristic1"
dbus-send --print-reply=literal --system --dest=org.bluez \
  /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a \
  org.freedesktop.DBus.Properties.Get \
  string:"org.bluez.GattCharacteristic1"\
  string:"Value"
```
Found that the d-bus value would not update, unless I requested a read in 
```
attribute-info /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a
select-attribute /org/bluez/hci0/dev_34_94_54_35_1D_F6/service0029/char002a
read
```
Need to enable notify
```
notify on
```
Walk BlueTooth Devices
```
udevadm info --attribute-walk /sys/class/bluetooth/hci0
```
Monitor udev
```
udevadm monitor --environment --udev
```
```
leet@thor:~ $ udevadm info -a -n /dev/input/event2

Udevadm info starts with the device specified by the devpath and then
walks up the chain of parent devices. It prints for every device
found, all possible attributes in the udev rules key format.
A rule to match, can be composed by the attributes of the device
and the attributes from one single parent device.

  looking at device '/devices/virtual/misc/uhid/0005:E502:ABCD.0188/input/input393/event2':
    KERNEL=="event2"
    SUBSYSTEM=="input"
    DRIVER==""
    ATTR{power/control}=="auto"
    ATTR{power/runtime_active_time}=="0"
    ATTR{power/runtime_status}=="unsupported"
    ATTR{power/runtime_suspended_time}=="0"

  looking at parent device '/devices/virtual/misc/uhid/0005:E502:ABCD.0188/input/input393':
    KERNELS=="input393"
    SUBSYSTEMS=="input"
    DRIVERS==""
    ATTRS{capabilities/abs}=="3007f"
    ATTRS{capabilities/ev}=="1b"
    ATTRS{capabilities/ff}=="0"
    ATTRS{capabilities/key}=="ffffffffffff 0 0 0 0 0 0 ffff000000000000 0 0 0 0"
    ATTRS{capabilities/led}=="0"
    ATTRS{capabilities/msc}=="10"
    ATTRS{capabilities/rel}=="0"
    ATTRS{capabilities/snd}=="0"
    ATTRS{capabilities/sw}=="0"
    ATTRS{id/bustype}=="0005"
    ATTRS{id/product}=="abcd"
    ATTRS{id/vendor}=="e502"
    ATTRS{id/version}=="1001"
    ATTRS{inhibited}=="0"
    ATTRS{name}=="ESP32TestOrg"
    ATTRS{phys}=="dc:a6:32:c2:ae:71"
    ATTRS{power/control}=="auto"
    ATTRS{power/runtime_active_time}=="0"
    ATTRS{power/runtime_status}=="unsupported"
    ATTRS{power/runtime_suspended_time}=="0"
    ATTRS{properties}=="0"
    ATTRS{uniq}=="34:94:54:35:1d:f6"

  looking at parent device '/devices/virtual/misc/uhid/0005:E502:ABCD.0188':
    KERNELS=="0005:E502:ABCD.0188"
    SUBSYSTEMS=="hid"
    DRIVERS=="hid-generic"
    ATTRS{country}=="00"
    ATTRS{power/control}=="auto"
    ATTRS{power/runtime_active_time}=="0"
    ATTRS{power/runtime_status}=="unsupported"
    ATTRS{power/runtime_suspended_time}=="0"

  looking at parent device '/devices/virtual/misc/uhid':
    KERNELS=="uhid"
    SUBSYSTEMS=="misc"
    DRIVERS==""
    ATTRS{power/control}=="auto"
    ATTRS{power/runtime_active_time}=="0"
    ATTRS{power/runtime_status}=="unsupported"
    ATTRS{power/runtime_suspended_time}=="0"

```
Should then see a joystick endpoint ```/dev/input/js0```
Joystick test using the following
```bash
sudo jstest /dev/input/js0
```
Some really cool info - https://thehackerdiary.wordpress.com/2017/04/21/exploring-devinput-1/
Can get info for input devices ```cat /proc/bus/input/devices```
```bash

I: Bus=0005 Vendor=e502 Product=abcd Version=1001
N: Name="ESP32TestOrg"
P: Phys=dc:a6:32:c2:ae:71
S: Sysfs=/devices/virtual/misc/uhid/0005:E502:ABCD.012F/input/input304
U: Uniq=34:94:54:35:1d:f6
H: Handlers=event2 js0
B: PROP=0
B: EV=1b
B: KEY=ffffffffffff 0 0 0 0 0 0 ffff000000000000 0 0 0 0
B: ABS=3007f
B: MSC=10
```
Noting the input ```event``` ID and ```js``` ID from the ```Handerls```
or just ```sudo evtest``` would get you
```bash
No device specified, trying to scan all of /dev/input/event*
Available devices:
/dev/input/event0:	vc4
/dev/input/event1:	vc4
/dev/input/event2:	ESP32TestOrg
```
then
```
leet@thor:~ $ evdev-joystick --s /dev/input/event2
Supported Absolute axes:
  Absolute axis 0x00 (0) (X Axis) (value: 0, min: 0, max: 32767, flatness: 2047 (=6.25%), fuzz: 127)
  Absolute axis 0x01 (1) (Y Axis) (value: 0, min: 0, max: 32767, flatness: 2047 (=6.25%), fuzz: 127)
  Absolute axis 0x02 (2) (Z Axis) (value: 0, min: 0, max: 32767, flatness: 2047 (=6.25%), fuzz: 127)
  Absolute axis 0x03 (3) (X Rate Axis) (value: 0, min: 0, max: 32767, flatness: 2047 (=6.25%), fuzz: 127)
  Absolute axis 0x04 (4) (Y Rate Axis) (value: 0, min: 0, max: 32767, flatness: 2047 (=6.25%), fuzz: 127)
  Absolute axis 0x05 (5) (Z Rate Axis) (value: 0, min: 0, max: 32767, flatness: 2047 (=6.25%), fuzz: 127)
  Absolute axis 0x06 (6) (Throttle) (value: 0, min: 0, max: 32767, flatness: 2047 (=6.25%), fuzz: 127)
  Absolute axis 0x10 (16) (Hat zero, x axis) (value: 0, min: -1, max: 1, flatness: 0 (=0.00%), fuzz: 0)
  Absolute axis 0x11 (17) (Hat zero, y axis) (value: 0, min: -1, max: 1, flatness: 0 (=0.00%), fuzz: 0)
```
fftesting could be done with
```
leet@thor:~ $ fftest /dev/input/event2
Force feedback test program.
HOLD FIRMLY YOUR WHEEL OR JOYSTICK TO PREVENT DAMAGES

Device /dev/input/event2 opened
Features:
  * Absolute axes: X, Y, Z, RX, RY, RZ, Throttle, Hat 0 X, Hat 0 Y,
    [7F 00 03 00 00 00 00 00 ]
  * Relative axes:
    [00 00 ]
  * Force feedback effects types:
    Force feedback periodic effects:
    [00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ]
  * Number of simultaneous effects: 0

Uploading effect #0 (Periodic sinusoidal) ... Error:: Function not implemented
Uploading effect #1 (Constant) ... Error: Function not implemented
Uploading effect #2 (Spring) ... Error: Function not implemented
Uploading effect #3 (Damper) ... Error: Function not implemented
Uploading effect #4 (Strong rumble, with heavy motor) ... Error: Function not implemented
Uploading effect #5 (Weak rumble, with light motor) ... Error: Function not implemented
Enter effect number, -1 to exit
```
Should be able to get the battery level with
```bash
dbus-send --print-reply=literal --system \
  --dest=org.bluez /org/bluez/hci0/dev_34_94_54_35_1D_F6 \
  org.freedesktop.DBus.Properties.Get string:"org.bluez.Battery1" \
  string:"Percentage"
```
```
   variant       byte 44
```
Display BLE serial number:
```bash
dbus-send --print-reply=literal --system \
  --dest=org.bluez /org/bluez/hci0/dev_34_94_54_35_1D_F6/service000a/char0013 \
  org.freedesktop.DBus.Properties.Get \
  string:"org.bluez.GattCharacteristic1" \
  string:"Value"
```
```
   variant       array of bytes "9876543210"
```

```bash
sudo gatttool -b 28:37:37:1A:D3:CF -I
```

```bash
sudo ./hidapitester --vidpid E502:ABCD -l 2 --open --send-output 3,5
Opening device, vid/pid: 0xE502/0xABCD
Writing output report of 2-bytes...wrote 2 bytes:
 03 05
Closing device
```

```
< ACL Data TX: Handle 64 flags 0x00 dlen 8                                                    #18 [hci0] 2.573784
      ATT: Write Request (0x12) len 3
        Handle: 0x0027
          Data: 05
```
Feedback over serial:
```
18:00:12.531 -> D NimBLECharacteristic: Characteristic 0x2a4d Write event
18:00:12.531 -> D NimBLECharacteristic: >> setValue: length=1, data=05, characteristic UUID=0x2a4d
18:00:12.531 -> D NimBLECharacteristic: << setValue
18:00:12.531 -> [7877892][I][BleGamepadOutputCallbacks.cpp:19] onWrite(): [BLEGamepad] leds: 5
18:00:12.531 -> [7877899][I][BleGamepadOutputCallbacks.cpp:20] onWrite(): [BLEGamepad] Callbacks written to
18:00:12.568 -> D NimBLECharacteristicCallbacks: onWrite: default
```
