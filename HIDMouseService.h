#ifndef BLE_HID_MOUSE_SERVICE_H__
#define BLE_HID_MOUSE_SERVICE_H__

#if BLE_FEATURE_GATT_SERVER

#include "HIDService.h"

/* -------------------------------------------------------------------------- */

namespace {

// Input Report
#pragma pack(push, 1)
struct {
  uint8_t buttons;
  uint8_t x;
  uint8_t y;
} hid_input_report;
#pragma pack(pop)

// Input report reference
static report_reference_t input_report_ref = { 0, INPUT_REPORT };
static GattAttribute input_report_ref_desc(
  BLE_UUID_REPORT_REF_DESCR,
  (uint8_t*)&input_report_ref,
  sizeof(input_report_ref),
  sizeof(input_report_ref)
);
static GattAttribute *input_report_ref_descs[] = {
  &input_report_ref_desc,
};

// Report Map
// (Example mouse descriptor from USB HID reference)
static uint8_t hid_report_map[] =
{
  USAGE_PAGE(1),      0x01,       // Usage Page (Generic Desktop)
  USAGE(1),           0x02,       // Usage (Mouse)
  COLLECTION(1),      0x01,       // Collection (Application)
    USAGE(1),           0x01,       // Usage (Pointer)
    COLLECTION(1),      0x00,       // Collection (Physical)
      // Buttons
      USAGE_PAGE(1),      0x09,       // Usage Page (Buttons)
      USAGE_MINIMUM(1),   0x01,       // Usage Minimum (1)
      USAGE_MAXIMUM(1),   0x03,       // Usage Maximum (3)
      LOGICAL_MINIMUM(1), 0x00,       // Logical Minimum (0)
      LOGICAL_MAXIMUM(1), 0x01,       // Logical Maximum (1)
      REPORT_COUNT(1),    0x03,       // Report Count (3)
      REPORT_SIZE(1),     0x01,       // Report Size (1)
      INPUT(1),           0x02,       // Input (Data, Variable, Absolute)
      // (padding)
      REPORT_COUNT(1),    0x01,       // Report Count (1)
      REPORT_SIZE(1),     0x05,       // Report Size (5)
      INPUT(1),           0x01,       // Input (Constant) for padding
      // Coordinates
      USAGE_PAGE(1),      0x01,       // Usage Page (Generic Desktop)
      USAGE(1),           0x30,       // Usage (X)
      USAGE(1),           0x31,       // Usage (Y)
      LOGICAL_MINIMUM(1), 0x81,       // Logical Minimum (-127)
      LOGICAL_MAXIMUM(1), 0x7f,       // Logical Maximum (+127)
      REPORT_SIZE(1),     0x08,       // Report Size (8)
      REPORT_COUNT(1),    0x02,       // Report Count (2)
      INPUT(1),           0x06,       // Input (Data, Variable, Relative)
    END_COLLECTION(0),              // End Collection (Physical)
  END_COLLECTION(0),              // End Collection (Application)
};

} // namespace "" 

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

  HIDMouseService(BLE &_ble) : 
    HIDService(_ble,
               HID_MOUSE,
               // report map
               hid_report_map, 
               sizeof(hid_report_map) / sizeof(*hid_report_map),
               // input report
               (uint8_t*)&hid_input_report,
               sizeof(hid_input_report),
               input_report_ref_descs,
               sizeof(input_report_ref_descs) / sizeof(*input_report_ref_descs))
  {}

  void motion(float fx, float fy) {
    uint8_t x = int(0x100 + fx * 0x7f) & 0xff;
    uint8_t y = int(0x100 + fy * 0x7f) & 0xff;

    // [debug] capture the first signal sent to remove conversion noise.
    static uint8_t xx = x;
    static uint8_t yy = y;
    x = uint8_t(x - xx) & 0xff;
    y = uint8_t(y - yy) & 0xff;

    hid_input_report.x = x;
    hid_input_report.y = y;
  }

  void button(Button buttons) {
    hid_input_report.buttons = uint8_t(buttons); 
  }
};

/* -------------------------------------------------------------------------- */

#endif // BLE_FEATURE_GATT_SERVER

#endif // BLE_HID_MOUSE_SERVICE_H__
