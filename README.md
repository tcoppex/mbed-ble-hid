# Mbed BLE HID

This project provides a simple library to implement *Human Interface Device* (**HID**) for *Bluetooth Low Energy* (**BLE**) on a Mbed stack using the *HID Over GATT Profile* (**HOGP**). 

It was designed for the **Arduino nano 33 BLE** and tested on _GNU/Linux, Android 8.0, and Windows 10_.

## Getting started

To develop your own HID you will need to derive the **HIDService** class and define your own `report descriptors` as described in the reference documentations. 

The classes **HIDGamepadService**, **HIDKeyboardService**, and **HIDMouseService** provide simple working-case demonstrations for Gamepad, Keyboard and Mouse HID, that you might want to improve upon for your project.

## Example

The available sample emulate a simple two-buttons mouse using an `Arduino nano 33 BLE` and an `analog 2-axis joystick` with the X axis (*respectively Y*) set to analog input **6** (*respectively 7*) and the push button set to digital input **2**.

By default the sample is set to *demo mode* and will output random motions for a few seconds after pairing. 
To disable *demo mode* you can set the macro definition **DEMO_ENABLE_RANDOM_INPUT** to 0.

## Known issues

On the Arduino IDE you might need the *MBED SDK* on your path to build, alternatively you can use [platformio](https://github.com/platformio) / [Deviot](https://github.com/platformio/Deviot) to resolve this automatically.

## Acknowledgment

This project has benefited from the following resources :

* mbed Microcontroller Library samples
* [Nordic semiconductor SDK 16](http://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v16.x.x/)
* [BLEKeyboard](https://github.com/bitbank2/BLE_Keyboard) by bitbank2
* [BLE_HID](https://github.com/jpbrucker/BLE_HID) by jpbrucker

You might want to look at jpbrucker's implementation for a well documented but deprecated alternative.

## References

* *Bluetooth HUMAN INTERFACE DEVICE PROFILE 1.1*
* *Bluetooth HID OVER GATT PROFILE SPECIFICATION v10*
* *USB Device Class Definition for Human Interface Devices (HID) v1.11* 
* *USB HID Usage Table v1.12*

## License

_**Mbed BLE HID**_ is released under the MIT License.
