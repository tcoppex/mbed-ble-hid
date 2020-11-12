# mbed BLE HID

This project provides a simple library to implement *Human Interface Device* (**HID**) for *Bluetooth Low Energy* (**BLE**) on a MBED stack using the *HID Over GATT Profile* (**HOGP**). 

It was designed for the **Arduino nano 33 BLE** and tested on _GNU/Linux, Android 8.0, and Windows 10_.

## Getting started

To develop your own HID you will need to derive the **HIDService** class and defines your own `report descriptors` as described in referenced documentation. 

The classes **HIDGamepadService**, **HIDKeyboardService**, and **HIDMouseService** provide simple working-cases demonstrations for Gamepad, Keyboard and Mouse respectively, that you might want to improve upon for your own project.

## Example

The available sample emulate a simple two-buttons mouse (motion and button states). 

By default the sample is set to *demo mode* and will output random motion for a few seconds after pairing. To disable *demo mode* you can set the macro definition **ENABLE_RANDOM_INPUT** to 0.

The demo was designed using an `Arduino nano 33 BLE` and an `analog 2-axis joystick` with its X axis (*respectively Y*) set to analog input **6** (*respectively 7*) and its push button set to digital input **1**.

## Known issue

On the Arduino IDE you might need the *MBED SDK* on your path to build, alternatively you can use [platformio](https://github.com/platformio) / [Deviot](https://github.com/platformio/Deviot) to resolve this automatically.

## Acknowledgement

This project has greatly benefited from the following resources :

* mbed Microcontroller Library samples
* [Nordic semiconductor SDK 16](http://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v16.x.x/)
* [BLEKeyboard](https://github.com/bitbank2/BLE_Keyboard) by bitbank2
* [BLE_HID](https://github.com/jpbrucker/BLE_HID) by jpbrucker

I recommend looking at jpbrucker's implementation for a well documented deprecated alternative.

## References

* *Bluetooth HUMAN INTERFACE DEVICE PROFILE 1.1*
* *Bluetooth HID OVER GATT PROFILE SPECIFICATION v10*
* *USB Device Class Definition for Human Interface Devices (HID) v1.11* 
* *USB HID Usage Table v1.12*

## License

_**mbed BLE HID**_ is released under the MIT License.
