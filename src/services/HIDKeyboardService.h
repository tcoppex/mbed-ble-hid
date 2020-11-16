#ifndef BLE_HID_KEYBOARD_SERVICE_H__
#define BLE_HID_KEYBOARD_SERVICE_H__

#if BLE_FEATURE_GATT_SERVER

#include "services/HIDService.h"

/* -------------------------------------------------------------------------- */

/* Ascii index of special function char. */
enum FunctionKeyChar_t {
    KEY_F1 = 128,       /* F1 key */
    KEY_F2,             /* F2 key */
    KEY_F3,             /* F3 key */
    KEY_F4,             /* F4 key */
    KEY_F5,             /* F5 key */
    KEY_F6,             /* F6 key */
    KEY_F7,             /* F7 key */
    KEY_F8,             /* F8 key */
    KEY_F9,             /* F9 key */
    KEY_F10,            /* F10 key */
    KEY_F11,            /* F11 key */
    KEY_F12,            /* F12 key */
 
    KEY_PRINT_SCREEN,   /* Print Screen key */
    KEY_SCROLL_LOCK,    /* Scroll lock */
    KEY_CAPS_LOCK,      /* caps lock */
    KEY_NUM_LOCK,       /* num lock */
    KEY_INSERT,         /* Insert key */
    KEY_HOME,           /* Home key */
    KEY_PAGE_UP,        /* Page Up key */
    KEY_PAGE_DOWN,      /* Page Down key */
 
    RIGHT_ARROW,        /* Right arrow */
    LEFT_ARROW,         /* Left arrow */
    DOWN_ARROW,         /* Down arrow */
    UP_ARROW,           /* Up arrow */
};

typedef uint16_t KeyCode_t;

/* Represent a key that could be sent by the HID. */
struct KeySym_t {
  enum Modifier {
    KEY_NONE  = 0,
    KEY_CTRL  = 1 << 0,
    KEY_SHIFT = 1 << 1,
    KEY_ALT   = 1 << 2,
  };

  /* Construct a Keysym directly by raw values. */
  KeySym_t(uint8_t _usage, uint8_t _modifiers) 
    : usage(_usage)
    , modifiers(_modifiers)
  {}

  /* Construct a Keysym from a specific keycode. */
  explicit KeySym_t(KeyCode_t _keycode);

  uint8_t usage;
  uint8_t modifiers;
};

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
  HIDKeyboardService(BLE &_ble);

  ble::adv_data_appearance_t appearance() const override {
    return ble::adv_data_appearance_t::KEYBOARD;
  }

  /* Return the KeySym for a specific character. */
  KeySym_t charToKeySym(unsigned char c) const;

  /* Send a press & release report for a single character. */
  void sendCharacter(unsigned char c);

  /* Register a down report for the specific keysym. */
  void keydown(KeySym_t keysym);

  /* Register a release report. */
  void keyup();
};

/* -------------------------------------------------------------------------- */

#endif // BLE_FEATURE_GATT_SERVER

#endif // BLE_HID_KEYBOARD_SERVICE_H__
