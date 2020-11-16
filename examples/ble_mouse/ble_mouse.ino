///
///   ble_mouse.ino
///
///   created: 2020-05
///   
///  Create a wireless Human Interface Device (HID) using the Bluetooth Low-Energy (BLE) 
///  HID-over-GATT Profile (HOGP) on a mbed stack (Arduino nano 33 BLE).
///

#include "Nano33BleHID.h"
#include "AnalogJoystick.h"
#include "signal_utils.h"

#define DEMO_ENABLE_RANDOM_INPUT        1
#define DEMO_DURATION_MS                4200

/* -------------------------------------------------------------------------- */

Nano33BleMouse bleMouse("nano33BLE Mouse");

// Analog Joystick wrapper, used to simulate a mouse.
AnalogJoystick gJoystick(A7, A6, 2);
static const float kJoystickSensibility = 0.125f;

// Builtin LED animation delays when disconnect. 
static const int kLedBeaconDelayMilliseconds = 1250;
static const int kLedErrorDelayMilliseconds = kLedBeaconDelayMilliseconds / 10;

// Builtin LED intensity when connected.
static const int kLedConnectedIntensity = 30;

/* -------------------------------------------------------------------------- */

void setup()
{
  // General setup.
  pinMode(LED_BUILTIN, OUTPUT);
  gJoystick.initialize();

  // Initialize both BLE and the HID.
  bleMouse.initialize();

  // Launch the event queue that will manage both BLE events and the loop. 
  // After this call the main thread will be halted.
  MbedBleHID_RunEventThread();
}

void loop()
{
  // When disconnected, we animate the builtin LED to indicate the device state.
  if (bleMouse.connected() == false) {
    animateLED(LED_BUILTIN, (bleMouse.has_error()) ? kLedErrorDelayMilliseconds 
                                                   : kLedBeaconDelayMilliseconds);
    return;
  }

  // When connected, we slightly dim the builtin LED.
  analogWrite(LED_BUILTIN, kLedConnectedIntensity);
  
  // Read the analog joystick inputs.
  gJoystick.update();
  float fx = kJoystickSensibility * gJoystick.x();
  float fy = kJoystickSensibility * gJoystick.y();
  auto buttons = gJoystick.button() ? HIDMouseService::BUTTON_LEFT 
                                    : HIDMouseService::BUTTON_NONE;

  // When demo mode is enabled we bypass the captured values 
  // to output random motion for a few seconds instead.
#if DEMO_ENABLE_RANDOM_INPUT
  if (bleMouse.connection_time() < DEMO_DURATION_MS)
  {
    fx = kJoystickSensibility * randf(-1.0f, 1.0f);
    fy = kJoystickSensibility * randf(-1.0f, 1.0f);
    buttons = HIDMouseService::BUTTON_NONE;
  }
  else
  {
    fx = 0.0f;
    fy = 0.0f;
  }
#endif

  // Update the HID report.
  auto *mouse = bleMouse.hid();
  mouse->motion(fx, fy);
  mouse->button(buttons);
  mouse->SendReport();
}

/* -------------------------------------------------------------------------- */
