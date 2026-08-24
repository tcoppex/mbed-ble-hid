#include <mbed.h>
#include "services/HIDGamepadService.h"

namespace {

// Report Reference
static report_reference_t input_report_ref{ 0, INPUT_REPORT };

static GattAttribute input_report_ref_desc(
  ATT_UUID_HID_REPORT_ID_MAPPING,
  (uint8_t*)&input_report_ref,
  sizeof(input_report_ref),
  sizeof(input_report_ref)
);

static GattAttribute *input_report_ref_descs[]{
  &input_report_ref_desc,
};

// Input Report
#pragma pack(push, 1)
struct {
  uint8_t x;
  uint8_t y;
  uint8_t buttons;
} hid_input_report;
#pragma pack(pop)

// Report Map
static uint8_t hid_report_map[]{
  USAGE_PAGE(1),      0x01,                 // Usage Page (Generic Desktop)
  USAGE(1),           USAGE_GAMEPAD,        // Usage (Game Pad)
  COLLECTION(1),      0x01,                 // Collection (Application)
    USAGE(1),           0x01,       // Usage (Pointer)
    COLLECTION(1),      0x00,       // Collection (Physical)
      USAGE_PAGE(1),      0x01,       // Usage Page (Generic Desktop)
      USAGE(1),           0x30,       // Usage (X)
      USAGE(1),           0x31,       // Usage (Y)
      REPORT_SIZE(1),     0x08,       // Report Size (8)
      REPORT_COUNT(1),    0x02,       // Report Count (2)
      INPUT(1),           0x02,       // Input (Data, Variable, Absolute)

      // Buttons
      USAGE_PAGE(1),      0x09,       // Usage Page (Buttons)
      USAGE_MINIMUM(1),   0x01,       // Usage Minimum (1)
      USAGE_MAXIMUM(1),   0x04,       // Usage Maximum (4)
      LOGICAL_MINIMUM(1), 0x00,       // Logical Minimum (0)
      LOGICAL_MAXIMUM(1), 0x01,       // Logical Maximum (1)
      REPORT_COUNT(1),    0x04,       // Report Count (4)
      REPORT_SIZE(1),     0x01,       // Report Size (1)
      INPUT(1),           0x02,       // Input (Data, Variable, Absolute)
      // (padding)
      REPORT_COUNT(1),    0x01,       // Report Count (1)
      REPORT_SIZE(1),     0x04,       // Report Size (4)
      INPUT(1),           0x01,       // Input (Constant) for padding
      
    END_COLLECTION(0),              // End Collection (Physical)
  END_COLLECTION(0),              // End Collection (Application)
};

} // namespace "" 


HIDGamepadService::HIDGamepadService(BLE &_ble) 
  : HIDService(_ble,
               HID_OTHER,
               
               // report map
               hid_report_map, 
               sizeof(hid_report_map) / sizeof(*hid_report_map),

               // input report
               (uint8_t*)&hid_input_report,
               sizeof(hid_input_report),
               input_report_ref_descs,
               sizeof(input_report_ref_descs) / sizeof(*input_report_ref_descs))
{

}

void HIDGamepadService::motion(float fx, float fy) {
  const uint8_t x = static_cast<int>(0x100 + fx * 0x7f) & 0xff;
  const uint8_t y = static_cast<int>(0x100 + fy * 0x7f) & 0xff;

  hid_input_report.x = x;
  hid_input_report.y = y;
}

void HIDGamepadService::button(Button buttons) {
  hid_input_report.buttons = static_cast<uint8_t>(buttons); 
}
