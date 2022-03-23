#ifndef MBED_BLE_HID_SERVICES_HID_GAMEPAD_SERVICE_H_
#define MBED_BLE_HID_SERVICES_HID_GAMEPAD_SERVICE_H_

#if BLE_FEATURE_GATT_SERVER

#include "services/HIDService.h"

/* -------------------------------------------------------------------------- */

/**
 * BLE HID Game Pad Service
 *
 * @par usage
 *
 * When this class is instantiated, it adds a Game Pad HID service in 
 * the GattServer.
 *
 * The GamePad consist of a joystick for X and Y motion and 4 buttons.
 *
 * @attention Multiple instances of this hid service are not supported.
 * @see HIDService
 */
class HIDGamepadService : public HIDService {
 public:
  enum Button {
    BUTTON_NONE    = 0,
    BUTTON_A       = 1 << 0,
    BUTTON_B       = 1 << 1,
    BUTTON_X       = 1 << 2,
    BUTTON_Y       = 1 << 3,
  };

  HIDGamepadService(BLE &_ble);

  ble::adv_data_appearance_t appearance() const override {
    return ble::adv_data_appearance_t::GAMEPAD;
  }

  void motion(float fx, float fy);
  void button(Button buttons);
};

/* -------------------------------------------------------------------------- */

#endif // BLE_FEATURE_GATT_SERVER

#endif // MBED_BLE_HID_SERVICES_HID_GAMEPAD_SERVICE_H_
