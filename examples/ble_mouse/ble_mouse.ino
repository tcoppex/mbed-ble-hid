///
///   ble_mouse.ino
///
///   created: 2020-05
///   
///  Simulate a mouse using an analogic joystick on an Arduino nano 33 ble.
///
///  Pins :  
///     * A7 : joystick X.
///     * A6 : joystick Y.
///     * D2 : joystick button.
///

#include "Nano33BleHID.h"

#include "AnalogJoystick.h"
#include "signal_utils.h"

/* -------------------------------------------------------------------------- */

static constexpr bool kEnableDemoMode = true;
static constexpr uint32_t kDemoModeTotalRuntime = 4200;

/* -------------------------------------------------------------------------- */

Nano33BleMouse bleMouse("nano33BLE Mouse");

// Analog Joystick wrapper, used to simulate a mouse.
AnalogJoystick gJoystick(A7, A6, 2);
static const float kJoystickSensibility = 0.10f;

// Builtin LED animation delays when disconnect. 
static const uint32_t kLedBeaconDelayMilliseconds = 1250;
static const uint32_t kLedErrorDelayMilliseconds = kLedBeaconDelayMilliseconds / 10;

// Builtin LED intensity when connected.
static const int kLedConnectedIntensity = 30;

/* -------------------------------------------------------------------------- */

void setup()
{
  // General setup.
  pinMode(LED_BUILTIN, OUTPUT);
  gJoystick.initialize();
  Serial.begin(9600); // serial don't work before the loop thread..
  
  // Initialize both BLE and the HID.
  bleMouse.initialize();
  
  // Launch the event queue that will manage both BLE events and the loop. 
  // After this call the main thread will be halted.
  MbedBleHID_RunEventThread(); 
}

void loop()
{
  // When disconnected, we animate the builtin LED to indicate the device state.
  if (bleMouse.disconnected()) {
    animateLED(LED_BUILTIN, (bleMouse.has_error()) ? kLedErrorDelayMilliseconds 
                                                   : kLedBeaconDelayMilliseconds);
    return;
  }
//  printf("connection time : %d\n", bleMouse.connection_time());

  // When connected, we slightly dim the builtin LED.
  analogWrite(LED_BUILTIN, kLedConnectedIntensity);

  // --------

  // Mouse states.
  float fx = 0.0f;
  float fy = 0.0f;
  HIDMouseService::Button buttons = HIDMouseService::BUTTON_NONE;

  if constexpr(kEnableDemoMode)
  {
    // printf("connection time : %d %d\n", bleMouse.connection_time(), kDemoModeTotalRuntime);
  
    // When demo mode is enabled we output random motion valuesfor a few seconds.
    bool const bRunDemo = true && (bleMouse.connection_time() < kDemoModeTotalRuntime);
    if (bRunDemo)
    {
      fx = kJoystickSensibility * randf(-1.0f, 1.0f);
      fy = kJoystickSensibility * randf(-1.0f, 1.0f);
    } else {
      printf("Demo has ended.\n");
    }
  }
  else
  {
    // Otherwhise we read the analog joystick inputs.
    gJoystick.update();
    fx = kJoystickSensibility * gJoystick.x();
    fy = kJoystickSensibility * gJoystick.y();
    buttons = gJoystick.button() ? HIDMouseService::BUTTON_LEFT 
                                 : HIDMouseService::BUTTON_NONE
                                 ;
  }

  // --------

  auto *mouse = bleMouse.hid();

  // Update the HID report.
  mouse->motion(fx, fy);
  mouse->button(buttons);

  // Send report to the ble server.
  mouse->sendReport();
}

/* -------------------------------------------------------------------------- */