///
///   ble_mouse.ino
///   2020-05
///   tcoppex@pm.me
///   
///  Create a wireless Human Interface Device (HID) using the Bluetooth Low-Energy (BLE) 
///  HID-over-GATT Profile (HOGP) on a mbed stack (Arduino nano 33 BLE).
///
///

#include <mbed.h>

#include <DeviceInformationService.h>
#include <BatteryService.h>

#include "HIDMouseService.h"
#include "AnalogJoystick.h"

#define DEMO_ENABLE_RANDOM_INPUT      1
#define DEMO_DURATION_MS              4200

#define LED_BEACON_DURATION_MS          1250
#define LED_ERROR_DURATION_MS           (LED_BEACON_DURATION_MS / 10)

/* -------------------------------------------------------------------------- */

static const char kDeviceName[]           = "nano33BLE HID";
static const char kManufacturerName[]     = "Acme Interactive";
static const char kGenericVersionString[] = "1234";
static const int  kDefaultBatteryLevel    = 98;

static const float kJoystickSensibility   = 0.09f;

static const int kEventQueueSize          = 32 * EVENTS_EVENT_SIZE;
static events::EventQueue eventQueue(kEventQueueSize);

/* -------------------------------------------------------------------------- */

struct ble_hid_t : Gap::EventHandler 
{
  // The HID-over-GATT Profile (HOGP) needs those three services
  // for the device to be recognized as an HID.
  struct {
    DeviceInformationService *deviceInformation;
    BatteryService *battery;
    HIDService *hid;
  } services;

  unsigned long lastConnection = 0;
  bool connected = false;
  bool hasError = false;

  // --------------------------------------

  void initialize(BLE &ble)
  {
    /// Note :
    /// when bonding is enabled, subsequent pairing after the initial one
    /// all fails, it might be a security parameters issue. 

    // Initialized Security manager with no Man-in-the-middle (MITM) protection.
    ble.securityManager().init(
      false,      // enable bonding.
      false,      // disable MITM protection.
      SecurityManager::IO_CAPS_NONE
    );

    // Add BLE services for the HID-over-GATT Profile (HOGP).
    services.deviceInformation = new DeviceInformationService(ble,
      kManufacturerName,
      kGenericVersionString,    // Model Number
      kGenericVersionString,    // Serial Number
      kGenericVersionString,    // Hardware Revision
      kGenericVersionString,    // Firmware Revision
      kGenericVersionString     // Software Revision
    );
    services.battery = new BatteryService(ble, kDefaultBatteryLevel);
    services.hid = new HIDMouseService(ble);
    
    // GAP events callbacks.
    Gap &gap = ble.gap();
    gap.setEventHandler(this);

    Gap::ConnectionParams_t params = {7, 15, 0, 3200};
    gap.setPreferredConnectionParams(&params);

    // GAP Advertising parameters.
    {
      using namespace ble;
      
      gap.setDeviceName((const uint8_t*)kDeviceName);
      gap.setAppearance(GapAdvertisingData::MOUSE);

      gap.setAdvertisingPayload(
        LEGACY_ADVERTISING_HANDLE,
        AdvertisingDataSimpleBuilder<LEGACY_ADVERTISING_MAX_SIZE>()
          .setFlags(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE)
          .setName(kDeviceName, true)
          .setAppearance(adv_data_appearance_t::MOUSE)
          .setLocalService(GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE)
          .getAdvertisingData()
      );

      gap.setAdvertisingParameters(
        LEGACY_ADVERTISING_HANDLE,
        AdvertisingParameters(advertising_type_t::CONNECTABLE_UNDIRECTED, adv_interval_t(millisecond_t(1000)))
          .setPrimaryInterval(conn_interval_t(millisecond_t(30)), conn_interval_t(millisecond_t(50)))
          .setOwnAddressType(own_address_type_t::RANDOM)
          .setPhy(phy_t::LE_1M, phy_t::LE_CODED)
      );
    }
  }

  void startAdvertising() {
    if (BLE::Instance().gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE)) {
      hasError = true;
    }
  }

  // -- EventHandler callbacks

  virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override
  {
    if (event.getStatus() == BLE_ERROR_NONE) {
      lastConnection = millis();
      connected = true;
      hasError  = false;
    } else {
      hasError = true;
    }
  }

  virtual void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override
  {
    connected = false;
    hasError  = false;
    startAdvertising();
  }
} gDevice;

// Analog Joystick helper used by the demo to simulate a mouse.
AnalogJoystick gJoystick(A7, A6, 2);

/* -------------------------------------------------------------------------- */

void bleScheduleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) 
{
  BLE &ble = BLE::Instance();
  eventQueue.call(mbed::Callback<void()>(&ble, &BLE::processEvents));
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
  if (params->error != BLE_ERROR_NONE) {
    gDevice.hasError = true;
    return;
  }
 
  BLE &ble = params->ble;  
  if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
    gDevice.hasError = true;
    return;
  }

  gDevice.initialize(ble);
  gDevice.startAdvertising();
}

/* -------------------------------------------------------------------------- */

void connectionUpdate() 
{
  // Capture analog joystick input.
  gJoystick.Read();

  // Post-process inputs.
  float fx = kJoystickSensibility * gJoystick.x();
  float fy = kJoystickSensibility * gJoystick.y();
  auto buttons = gJoystick.button() ? HIDMouseService::BUTTON_LEFT 
                                    : HIDMouseService::BUTTON_NONE;

  // When demo mode is enabled we bypassed the captured values to output random noises.
#if DEMO_ENABLE_RANDOM_INPUT
  if ((millis() - gDevice.lastConnection) < DEMO_DURATION_MS)
  {
    fx = kJoystickSensibility * randf();
    fy = kJoystickSensibility * randf();
    buttons = HIDMouseService::BUTTON_NONE;
  }
  else
  {
    fx = 0.0f;
    fy = 0.0f;
  }
#endif

  // Update and send the HID report.
  auto mouse = (HIDMouseService*)gDevice.services.hid;
  mouse->motion(fx, fy);
  mouse->button(buttons);
  mouse->SendReport();
}

/* Task used by the event thread, bypassing the usual Arduino loop method. */
void loopTask()
{
  // Update the builtin LED.
  if (!gDevice.connected) {
    if (gDevice.hasError) {
      // Quick beaconing signal an error.
      animateLED(LED_BUILTIN, LED_ERROR_DURATION_MS);
    } else {
      // Animate the main LED while searching for a connection.
      animateLED(LED_BUILTIN, LED_BEACON_DURATION_MS);
    } 
    return;
  }
  analogWrite(LED_BUILTIN, 30);
  
  connectionUpdate();
}

/* -------------------------------------------------------------------------- */

void setup()
{
  // App specific initializations.
  pinMode(LED_BUILTIN, OUTPUT);
  gJoystick.Calibrate();

  // ---

  // Initialize the BLE device.
  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(bleScheduleEventsProcessing);
  ble.init(bleInitComplete);

  // Bypass the Arduino loop for an update event call.
  eventQueue.call_every(10, loopTask);

  // Launch a new thread for handling events.
  rtos::Thread eventThread;
  eventThread.start(mbed::callback(&eventQueue, &events::EventQueue::dispatch_forever));

  // Put the main thread to sleep.
  rtos::ThisThread::sleep_for(osWaitForever);
}

void loop()
{
  // unused
}

/* -------------------------------------------------------------------------- */
