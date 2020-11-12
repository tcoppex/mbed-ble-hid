#ifndef BLE_HID_KEYBOARD_SERVICE_H__
#define BLE_HID_KEYBOARD_SERVICE_H__

#if BLE_FEATURE_GATT_SERVER

#include "HIDService.h"

/* -------------------------------------------------------------------------- */

namespace {

// Input Report
struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t key_codes[6];
} hid_input_report;

// Input Report Reference
static report_reference_t input_report_ref = { 0, INPUT_REPORT };
static GattAttribute input_report_ref_desc(
  ATT_UUID_HID_REPORT_ID_MAPPING,
  (uint8_t*)&input_report_ref,
  sizeof(input_report_ref),
  sizeof(input_report_ref)
);
static GattAttribute *input_report_ref_descs[] = {
  &input_report_ref_desc,
};

// Output Report
struct {
  uint8_t leds;
} hid_output_report;

// Output Report Reference
static report_reference_t output_report_ref = { 0, OUTPUT_REPORT };
static GattAttribute output_report_ref_desc(
  ATT_UUID_HID_REPORT_ID_MAPPING,
  (uint8_t*)&output_report_ref,
  sizeof(output_report_ref),
  sizeof(output_report_ref)
);
static GattAttribute *output_report_ref_descs[] = {
  &output_report_ref_desc,
};

// Report Map
// (Example keyboard descriptor from USB HID reference)
static uint8_t hid_report_map[] =
{
  USAGE_PAGE(1),      0x01,       // Usage Page (Generic Desktop)
  USAGE(1),           0x06,       // Usage (Keyboard)
  COLLECTION(1),      0x01,       // Collection (Application)
    // Key codes (Modifiers)
    USAGE_PAGE(1),      0x07,       // Usage Page (Key Codes)
    USAGE_MINIMUM(1),   0xE0,       // Usage Minimum (224)
    USAGE_MAXIMUM(1),   0xE7,       // Usage Maximum (231)
    LOGICAL_MINIMUM(1), 0x00,       // Logical Minimum (0)
    LOGICAL_MAXIMUM(1), 0x01,       // Logical Maximum (1)
    REPORT_COUNT(1),    0x08,       // Report Count (8)
    REPORT_SIZE(1),     0x01,       // Report Size (1)
    INPUT(1),           0x02,       // Input (Data, Variable, Absolute)
    // (reserved bits)
    REPORT_COUNT(1),    0x01,       // Report Count (1)
    REPORT_SIZE(1),     0x08,       // Report Size (8)
    INPUT(1),           0x01,       // Input (Constant)
    // Output LEDs
    USAGE_PAGE(1),      0x08,       // Usage Page (LEDs)
    USAGE_MINIMUM(1),   0x01,       // Usage Minimum (1)
    USAGE_MAXIMUM(1),   0x05,       // Usage Maximum (5)
    REPORT_COUNT(1),    0x05,       // Report Count (5)
    REPORT_SIZE(1),     0x01,       // Report Size (1)
    OUTPUT(1),          0x02,       // Output (Data, Variable, Absolute)
    // (output padding)
    REPORT_COUNT(1),    0x01,       // Report Count (1)
    REPORT_SIZE(1),     0x03,       // Report Size (3)
    OUTPUT(1),          0x01,       // Output (Constant)
    // Key codes
    USAGE_PAGE(1),      0x07,       // Usage Page (Key Codes)
    USAGE_MINIMUM(1),   0x00,       // Usage Minimum (0)
    USAGE_MAXIMUM(1),   0x65,       // Usage Maximum (101)
    LOGICAL_MINIMUM(1), 0x00,       // Logical Minimum (0)
    LOGICAL_MAXIMUM(1), 0x65,       // Logical Maximum (101)
    REPORT_COUNT(1),    0x06,       // Report Count (6)
    REPORT_SIZE(1),     0x08,       // Report Size (8)
    INPUT(1),           0x00,       // Input (Data, Array)
  END_COLLECTION(0),
};

} // namespace "" 

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
      NONE      = 0,
      KEY_CTRL  = 1 << 0,
      KEY_SHIFT = 1 << 1,
      KEY_ALT   = 1 << 2,
  };

  HIDKeyboardService(BLE &_ble) : 
    HIDService(_ble,
               HID_KEYBOARD,
               // report map
               hid_report_map, 
               sizeof(hid_report_map) / sizeof(*hid_report_map),
               // input report
               (uint8_t*)&hid_input_report,
               sizeof(hid_input_report),
               input_report_ref_descs,
               sizeof(input_report_ref_descs) / sizeof(*input_report_ref_descs),
               // output report
               (uint8_t*)&hid_output_report,
               sizeof(hid_output_report),
               output_report_ref_descs,
               sizeof(output_report_ref_descs) / sizeof(*output_report_ref_descs))
  {}

  void keydown(uint8_t key, Modifier modifiers = Modifier::NONE) {
    hid_input_report.modifiers    = modifiers;
    hid_input_report.key_codes[0] = key;
  }

  void keyup() {
    memset(&hid_input_report, 0, sizeof(hid_input_report));
  }
};

/* -------------------------------------------------------------------------- */

#endif // BLE_FEATURE_GATT_SERVER

#endif // BLE_HID_KEYBOARD_SERVICE_H__
