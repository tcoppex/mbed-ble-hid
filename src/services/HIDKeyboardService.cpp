#include <array>

#include <mbed.h>
#include "services/HIDKeyboardService.h"
#include "services/keylayouts.h"

/* -------------------------------------------------------------------------- */

namespace {

/* Map a character to its layout keycode. */
std::array<KeyCode_t, 182> s_keyLUT{
    0,             /* NUL */
    0,             /* SOH */
    0,             /* STX */
    0,             /* ETX */
    0,             /* EOT */
    0,             /* ENQ */
    0,             /* ACK */
    0,             /* BEL */
    KEYCODE_BACKSPACE,
    KEYCODE_TAB,
    KEYCODE_ENTER,
    0,             /* VT  */
    0,             /* FF  */
    0,             /* CR  */
    0,             /* SO  */
    0,             /* SI  */
    0,             /* DEL */
    0,             /* DC1 */
    0,             /* DC2 */
    0,             /* DC3 */
    0,             /* DC4 */
    0,             /* NAK */
    0,             /* SYN */
    0,             /* ETB */
    0,             /* CAN */
    0,             /* EM  */
    0,             /* SUB */
    KEYCODE_ESC,   /* ESC */
    0,             /* FS  */
    0,             /* GS  */
    0,             /* RS  */
    0,             /* US  */
    ASCII_20,      /*   */
    ASCII_21,      /* ! */
    ASCII_22,      /* " */
    ASCII_23,      /* # */
    ASCII_24,      /* $ */
    ASCII_25,      /* % */
    ASCII_26,      /* & */
    ASCII_27,      /* ' */
    ASCII_28,      /* ( */
    ASCII_29,      /* ) */
    ASCII_2A,      /* * */
    ASCII_2B,      /* + */
    ASCII_2C,      /* , */
    ASCII_2D,      /* - */
    ASCII_2E,      /* . */
    ASCII_2F,      /* / */
    ASCII_30,      /* 0 */
    ASCII_31,      /* 1 */
    ASCII_32,      /* 2 */
    ASCII_33,      /* 3 */
    ASCII_34,      /* 4 */
    ASCII_35,      /* 5 */
    ASCII_36,      /* 6 */
    ASCII_37,      /* 7 */
    ASCII_38,      /* 8 */
    ASCII_39,      /* 9 */
    ASCII_3A,      /* : */
    ASCII_3B,      /* ; */
    ASCII_3C,      /* < */
    ASCII_3D,      /* = */
    ASCII_3E,      /* > */
    ASCII_3F,      /* ? */
    ASCII_40,      /* @ */
    ASCII_41,      /* A */
    ASCII_42,      /* B */
    ASCII_43,      /* C */
    ASCII_44,      /* D */
    ASCII_45,      /* E */
    ASCII_46,      /* F */
    ASCII_47,      /* G */
    ASCII_48,      /* H */
    ASCII_49,      /* I */
    ASCII_4A,      /* J */
    ASCII_4B,      /* K */
    ASCII_4C,      /* L */
    ASCII_4D,      /* M */
    ASCII_4E,      /* N */
    ASCII_4F,      /* O */
    ASCII_50,      /* P */
    ASCII_51,      /* Q */
    ASCII_52,      /* R */
    ASCII_53,      /* S */
    ASCII_54,      /* T */
    ASCII_55,      /* U */
    ASCII_56,      /* V */
    ASCII_57,      /* W */
    ASCII_58,      /* X */
    ASCII_59,      /* Y */
    ASCII_5A,      /* Z */
    ASCII_5B,      /* [ */
    ASCII_5C,      /* \ */
    ASCII_5D,      /* ] */
    ASCII_5E,      /* ^ */
    ASCII_5F,      /* _ */
    ASCII_60,      /* ` */
    ASCII_61,      /* a */
    ASCII_62,      /* b */
    ASCII_63,      /* c */
    ASCII_64,      /* d */
    ASCII_65,      /* e */
    ASCII_66,      /* f */
    ASCII_67,      /* g */
    ASCII_68,      /* h */
    ASCII_69,      /* i */
    ASCII_6A,      /* j */
    ASCII_6B,      /* k */
    ASCII_6C,      /* l */
    ASCII_6D,      /* m */
    ASCII_6E,      /* n */
    ASCII_6F,      /* o */
    ASCII_70,      /* p */
    ASCII_71,      /* q */
    ASCII_72,      /* r */
    ASCII_73,      /* s */
    ASCII_74,      /* t */
    ASCII_75,      /* u */
    ASCII_76,      /* v */
    ASCII_77,      /* w */
    ASCII_78,      /* x */
    ASCII_79,      /* y */
    ASCII_7A,      /* z */
    ASCII_7B,      /*  */
    ASCII_7C,      /* | */
    ASCII_7D,      /* } */
    ASCII_7E,      /* ~ */
    ASCII_7F,      /* DEL */
 
    KEYCODE_F1,
    KEYCODE_F2,
    KEYCODE_F3,
    KEYCODE_F4,
    KEYCODE_F5,
    KEYCODE_F6,
    KEYCODE_F7,
    KEYCODE_F8,
    KEYCODE_F9,
    KEYCODE_F10,
    KEYCODE_F11,
    KEYCODE_F12,
    KEYCODE_PRINTSCREEN,
    KEYCODE_SCROLL_LOCK,
    KEYCODE_CAPS_LOCK,
    KEYCODE_NUM_LOCK,
    KEYCODE_INSERT,
    KEYCODE_HOME,
    KEYCODE_PAGE_UP,
    KEYCODE_PAGE_DOWN,
    KEYCODE_RIGHT,
    KEYCODE_LEFT,
    KEYCODE_DOWN,
    KEYCODE_UP,

    KEYCODE_KP_SLASH,
    KEYCODE_KP_ASTERIX,
    KEYCODE_KP_MINUS,
    KEYCODE_KP_PLUS,
    KEYCODE_KP_ENTER,
    KEYCODE_KP_1,
    KEYCODE_KP_2,
    KEYCODE_KP_3,
    KEYCODE_KP_4,
    KEYCODE_KP_5,
    KEYCODE_KP_6,
    KEYCODE_KP_7,
    KEYCODE_KP_8,
    KEYCODE_KP_9,
    KEYCODE_KP_0,
    KEYCODE_KP_PERIOD,
    KEYCODE_NON_US_BS,
    KEYCODE_MENU,
    KEYCODE_F13,
    KEYCODE_F14,
    KEYCODE_F15,
    KEYCODE_F16,
    KEYCODE_F17,
    KEYCODE_F18,
    KEYCODE_F19,
    KEYCODE_F20,
    KEYCODE_F21,
    KEYCODE_F22,
    KEYCODE_F23,
    KEYCODE_F24,
};

} // namespace ""

/* -------------------------------------------------------------------------- */

KeySym_t::KeySym_t(KeyCode_t _keycode)
  : usage(0)
  , modifiers(0)
{
  _keycode &= KEYCODE_MASK;
  
  if (_keycode > ALTGR_MASK) {
    _keycode -= ALTGR_MASK;
    modifiers |= Modifier::KEY_ALT;
  }
  if (_keycode > SHIFT_MASK) {
    _keycode -= SHIFT_MASK;
    modifiers |= Modifier::KEY_SHIFT;
  }

  usage = _keycode & 0xff;
}

/* -------------------------------------------------------------------------- */

namespace {

// Input Report
#pragma pack(push, 1)
struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t key_codes[6];
} hid_input_report;
#pragma pack(pop)

// Input Report Reference
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

// Output Report
struct {
  uint8_t leds;
} hid_output_report;

// Output Report Reference
static report_reference_t output_report_ref{ 0, OUTPUT_REPORT };

static GattAttribute output_report_ref_desc(
  ATT_UUID_HID_REPORT_ID_MAPPING,
  (uint8_t*)&output_report_ref,
  sizeof(output_report_ref),
  sizeof(output_report_ref)
);

static GattAttribute *output_report_ref_descs[]{
  &output_report_ref_desc,
};

// Report Map
// (Example keyboard descriptor from USB HID reference)
static uint8_t hid_report_map[]{
  USAGE_PAGE(1),      0x01,           // Usage Page (Generic Desktop)
  USAGE(1),           USAGE_KEYBOARD, // Usage (Keyboard)
  COLLECTION(1),      0x01,           // Collection (Application)
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

HIDKeyboardService::HIDKeyboardService(BLE &_ble) : 
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

KeySym_t HIDKeyboardService::charToKeySym(unsigned char c) const {
  const auto keycode = (c < s_keyLUT.size()) ? s_keyLUT[c] : DEADKEYS_MASK;
  return KeySym_t(keycode);
}

void HIDKeyboardService::sendCharacter(unsigned char c) {
  const auto keysym = charToKeySym(c);
  
  // KeyDown report.
  keydown(keysym);
  sendReport();

  // KeyUp report.
  keyup();
  sendReport(); 
}

void HIDKeyboardService::keydown(KeySym_t keysym) {
  hid_input_report.modifiers    = keysym.modifiers;
  hid_input_report.key_codes[0] = keysym.usage;
}

void HIDKeyboardService::keyup() {
  memset(&hid_input_report, 0, sizeof(hid_input_report));
}

/* -------------------------------------------------------------------------- */