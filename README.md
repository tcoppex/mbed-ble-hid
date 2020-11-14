# Mbed BLE HID

This project provides a simple library to implement *Human Interface Device* (**HID**) for *Bluetooth Low Energy* (**BLE**) on a Mbed stack using the *HID Over GATT Profile* (**HOGP**). 

It was designed for the **Arduino nano 33 BLE** and tested on _GNU/Linux, Android 8.0, and Windows 10_.

## Environment

On the Arduino IDE you will need the **Arduino nRF528x boards (Mbed OS)** with version **1.1.6** (In the menu bar click on "_Tools > Boards > Boards manager.._").

Alternatively you can also use [platformio](https://github.com/platformio) / [Deviot](https://github.com/platformio/Deviot).


## Getting started

The header `Nano33BleHID.h` defines three basic HID ready to use : Mouse, Keyboard, and Gamepad.

```cpp
#include "Nano33BleHID.H"

// Alias to Nano33BleHID<HIDGamepadService>
Nano33BleGamepad pad("SuperAwesome Pad");

void setup() {
    // initialize the ble HID.
    pad.initialize();

    // Launch the eventqueue thread.
    MbedBleHID_RunEventThread();
}

void loop() {
    // Retrieve the HID service handle.
    auto hid = pad.hid();

    // Update internal values.
    float theta = PI * (random(255) / 255.0);
    hid.motion(cos(theta), sin(theta));

    // Send them !
    hid.SendReport();
}
```

## Creating a custom HID

A bluetooth HID is defined by *at least* three services :
 * a *Device Information* service,
 * a *Battery* service,
 * a *HID* service.

This library defines the *device information* and *battery* services for you, as well as basic HID services for mouse, keyboard, and gamepad (**HIDMouseService**, **HIDKeyboardService**, and **HIDGamepadService** respectively).

To develop your own HID you will need to derive the **HIDService** class and define a `report descriptors` as described in referenced documentations. A _report descriptor_ is defined by a markup-like language that maps input values to standard interface, like a pointer motion or a key pressed.

Once you have written your HID service you have two options :

If your service has a single parameter constructor you can directly use the *Nano33BleHID<T>* wrapper template to create your bluetooth HID :
```cpp
#include "Nano33BleHID.h"

class HIDSampleService : HIDService {
  public:
    HIDSampleService(BLE &_ble);

    // Implement this method to describe how your HID 
    // is perceived by other BLE managers.
    ble::adv_data_appearance_t appearance() override; 
};

Nano33BleHID<HIDSampleService> sampleHID;
```

Alternatively you can derive your HID from the base class `MbedBleHID` for more complex cases :
```cpp
#include "MbedBleHID.h"

class SampleHID : MbedBleHID {
  public:
    SampleHID() : MbedBleHID("A Sample HID in the wild") {/**/}

    // This should return the polymorphic shared_ptr of your service.
    std::shared_ptr<HIDService> CreateHIDService(BLE &ble) override;

    // [ Add some fancy stuff here & there ]
};

SampleHID mySampleHID;
```


## Example

### ble_mouse

This sample emulate a simple two-buttons mouse (motion and button states), using an `Arduino nano 33 BLE` and an `analog 2-axis joystick` with its X axis (*respectively Y*) set to analog input **6** (*respectively 7*) and its push button set to digital input **1**.

By default the sample is set to *demo mode* and will output random motions for a few seconds after pairing.

To disable *demo mode* you can set the macro definition **DEMO_ENABLE_RANDOM_INPUT** to 0.

## Known limitations

*Boot protocol*, which allows for mouses and keyboards to be used on a basic boot system, are layed out but not implemented.

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
