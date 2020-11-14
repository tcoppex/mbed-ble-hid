#ifndef BLE_HID_KEYBOARD_SERVICE_H__
#define BLE_HID_KEYBOARD_SERVICE_H__

#if BLE_FEATURE_GATT_SERVER

#include "services/HIDService.h"

/* -------------------------------------------------------------------------- */

/**
 * BLE HID Keyboard Service
 *
 * @par usage
 *
 * When this class is instantiated, it adds a keyboard HID service in 
 * the GattServer.
 *
 * @attention Multiple instances of this hid service are not supported.
 * @see HIDService
 */
class HIDKeyboardService : public HIDService {
 public:
   enum Modifier {
      KEY_NONE  = 0,
      KEY_CTRL  = 1 << 0,
      KEY_SHIFT = 1 << 1,
      KEY_ALT   = 1 << 2,
  };

  HIDKeyboardService(BLE &_ble);

  ble::adv_data_appearance_t appearance() const override {
    return ble::adv_data_appearance_t::KEYBOARD;
  }

  void keydown(uint8_t key, Modifier modifiers = Modifier::KEY_NONE);
  void keyup();
};

/* -------------------------------------------------------------------------- */

#endif // BLE_FEATURE_GATT_SERVER

#endif // BLE_HID_KEYBOARD_SERVICE_H__
