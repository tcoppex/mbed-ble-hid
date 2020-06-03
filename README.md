# mbed BLE HID

This project provides a simple library to implement *Human Interface Device* (HID) 
for *Bluetooth Low Energy* (BLE) on a MBED stack using the *HID Over GATT Profile* (HOGP). 

It was especially developped for the **Arduino nano 33 BLE** and tested with GNU/Linux (*Linux Mint 19.3*), 
Android 8.0, and Windows 10.

## How To Use

To develop your own HID simply derives the **HIDService** and defines your own *report descriptors*
as described in referenced documentation. The classes *HIDGamepadService*, *HIDKeyboardService*, 
and *HIDMouseService* are simple working-cases for each of those devices that you might want to improve upon for
your own project.

## Example

The available example emulate a mouse using an analog 2-axis joystick and an *Arduino nano 33 BLE* 
with the X (respectively Y) axis set to analog input 6 (resp. 7) and the push button set to digital input 1.

If you don't have any analog joystick you can enable the **ENABLE_RANDOM_INPUT** define to test the sample.

## Known issues

The device only work after the initial coupling. A temporary fix consists of dissassociate the device 
from the server before coupling it again.

*Any help with that would be really appreciated.*

## Acknowledgement

This project greatly benefited from the following resources :

* mbed Microcontroller Library samples
* [Nordic semiconductor SDK 16](http://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v16.x.x/)
* [BLEKeyboard](https://github.com/bitbank2/BLE_Keyboard) by bitbank2
* [BLE_HID](https://github.com/jpbrucker/BLE_HID) by jpbrucker

I recommend looking at jpbrucker's implementation for a deprecated alternative really well
documented.

## References

* *Bluetooth HUMAN INTERFACE DEVICE PROFILE 1.1*
* *Bluetooth HID OVER GATT PROFILE SPECIFICATION v10*
* *USB Device Class Definition for Human Interface Devices (HID) v1.11* 
* *USB HID Usage Table v1.12*

## License

This project is released under the MIT License.