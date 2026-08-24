///
///   ble_gamepad.ino
///
///   created: 2024-10
///
///   Create a wireless Human Interface Device (HID) gamepad using the Bluetooth Low-Energy (BLE)
///   HID-over-GATT Profile (HOGP) on an Arduino Nano 33 BLE Sense Rev2.
///
///   This sketch simulates a gamepad that sends random joystick movements and button presses
///   to a connected PC.
///
///   No additional hardware is required.
///
/// --------------------------------------------------------------------------

#include "Nano33BleHID.h"
#include "signal_utils.h"

#define DEMO_ENABLE_RANDOM_INPUT        1
#define DEMO_DURATION_MS                5000

/* -------------------------------------------------------------------------- */

// Use the existing Nano33BleGamepad type alias
Nano33BleGamepad bleGamepad("nano33BLE Gamepad");

// Built-in LED animation delays when disconnected
static const int kLedBeaconDelayMilliseconds = 1250;
static const int kLedErrorDelayMilliseconds  = kLedBeaconDelayMilliseconds / 10;

// Built-in LED intensity when connected
static const int kLedConnectedIntensity = 30;

/* -------------------------------------------------------------------------- */

void setup()
{
  // General setup
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize BLE and the HID service
  bleGamepad.initialize();

  // Launch the event queue that will manage BLE events and the loop
  // After this call, the main thread will be halted
  MbedBleHID_RunEventThread();
}

/* -------------------------------------------------------------------------- */

void loop()
{
  // When disconnected, animate the built-in LED to indicate the device state
  if (!bleGamepad.connected()) {
    animateLED(LED_BUILTIN, bleGamepad.has_error() ? kLedErrorDelayMilliseconds
                                                   : kLedBeaconDelayMilliseconds);
    return;
  }

  // When connected, slightly dim the built-in LED
  analogWrite(LED_BUILTIN, kLedConnectedIntensity);

  // Retrieve the HID Gamepad service
  auto *gamepad = bleGamepad.hid();

  // For demo purposes, send random joystick values and button presses
#if DEMO_ENABLE_RANDOM_INPUT
  bool const bPlayDemo = true; // You can add conditions if needed
  if (bPlayDemo)
  {
    // Random joystick movement between -1.0 and 1.0
    float fx = randf(-1.0f, 1.0f);
    float fy = randf(-1.0f, 1.0f);

    // Randomly select buttons to press
    int randomButton = rand() % 5; // Values from 0 to 4
    HIDGamepadService::Button buttons;
    switch (randomButton) {
      case 0:
        buttons = HIDGamepadService::BUTTON_NONE;
        break;
      case 1:
        buttons = HIDGamepadService::BUTTON_A;
        break;
      case 2:
        buttons = HIDGamepadService::BUTTON_B;
        break;
      case 3:
        buttons = HIDGamepadService::BUTTON_X;
        break;
      case 4:
        buttons = HIDGamepadService::BUTTON_Y;
        break;
    }

    // Update the HID report with the random values
    gamepad->motion(fx, fy);
    gamepad->button(buttons);
    gamepad->SendReport();
  }
#else
  // Optionally, you can add code to handle actual inputs here
#endif
}

/* -------------------------------------------------------------------------- */
