#ifndef MBED_BLE_HID_SERVICES_HID_MOUSE_SERVICE_H_
#define MBED_BLE_HID_SERVICES_HID_MOUSE_SERVICE_H_

#if BLE_FEATURE_GATT_SERVER

#include "services/HIDService.h"

/* -------------------------------------------------------------------------- */

/**
 * BLE HID Mouse Service
 *
 * @par usage
 *
 * When this class is instantiated, it adds a mouse HID service in 
 * the GattServer.
 *
 * @attention Multiple instances of this hid service are not supported.
 * @see HIDService
 */
class HIDMouseService : public HIDService {
 public:
  enum Button {
    BUTTON_NONE    = 0,
    BUTTON_LEFT    = 1 << 0,
    BUTTON_RIGHT   = 1 << 1,
    BUTTON_MIDDLE  = 1 << 2,
  };

  HIDMouseService(BLE &_ble);

  ble::adv_data_appearance_t appearance() const override {
    return ble::adv_data_appearance_t::MOUSE;
  }

  void motion(float fx, float fy);

  void button(Button buttons);
};

/* -------------------------------------------------------------------------- */

#endif // BLE_FEATURE_GATT_SERVER

#endif // MBED_BLE_HID_SERVICES_HID_MOUSE_SERVICE_H_
