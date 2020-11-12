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

#define DEMO_ENABLE_RANDOM_INPUT        1
#define DEMO_DURATION_MS                4200

/* -------------------------------------------------------------------------- */

static const char kDeviceName[]           = "nano33BLE HID";
static const char kManufacturerName[]     = "Acme Interactive";
static const char kGenericVersionString[] = "1234";
static const int  kDefaultBatteryLevel    = 98;

static const int kLedBeaconDelayMilliseconds = 1250;
static const int kLedErrorDelayMilliseconds  = kLedBeaconDelayMilliseconds / 10;
static const float kJoystickSensibility      = 0.125f;

static const int kEventQueueSize = 16 * EVENTS_EVENT_SIZE;
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

  // -- EventHandler callbacks

  virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override
  {
    hasError = (BLE_ERROR_NONE != event.getStatus());
    connected = !hasError;
    if (connected) {
      lastConnection = millis();
    }
  }

  virtual void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override
  {
    hasError  = false;
    connected = false;
    startAdvertising();
  }

  // -- Utility

  void setupServices(BLE &ble)
  {
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
  }

  void startAdvertising() {
    if (BLE::Instance().gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE)) {
      hasError = true;
    }
  }

} gDevice;

// Analog Joystick helper used by the demo to simulate a mouse.
AnalogJoystick gJoystick(A7, A6, 2);

/* -------------------------------------------------------------------------- */

/* BLE events scheduling Callback */
void bleScheduleEventsProcessing_cb(BLE::OnEventsToProcessCallbackContext* context) 
{
  BLE &ble = BLE::Instance();
  eventQueue.call(mbed::Callback<void()>(&ble, &BLE::processEvents));
}

/* Post BLE initialization Callback */
void bleInitComplete_cb(BLE::InitializationCompleteCallbackContext *params)
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

  // Initialized Security manager with no Man-in-the-middle (MITM) protection.
  // @note: When bonding is enabled subsequent pairing after the initial one
  //        fail, it might be a security parameters issue. 
  ble.securityManager().init(
    false,      // disable bonding.
    false,      // disable MITM protection.
    SecurityManager::IO_CAPS_NONE
  );

  // Setup BLE services.
  gDevice.setupServices(ble);

  // GAP events callbacks.
  Gap &gap = ble.gap();
  gap.setEventHandler(&gDevice);

  const Gap::ConnectionParams_t connectionParams = {
    7,    // min conn interval
    15,   // max conn interval
    0,    // slave latency
    3200  // supervision timeout
  };
  gap.setPreferredConnectionParams(&connectionParams);

  // GAP Advertising parameters.
  {
    using namespace ble;
    
    gap.setDeviceName((const uint8_t*)kDeviceName);
    gap.setAppearance(GapAdvertisingData::MOUSE);

    gap.setAdvertisingPayload(
      LEGACY_ADVERTISING_HANDLE,
      AdvertisingDataSimpleBuilder<LEGACY_ADVERTISING_MAX_SIZE>()
        .setFlags(GapAdvertisingData::BREDR_NOT_SUPPORTED 
                | GapAdvertisingData::LE_GENERAL_DISCOVERABLE
        )
        .setName(kDeviceName, true)
        .setAppearance(adv_data_appearance_t::MOUSE)
        .setLocalService(GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE)
        .getAdvertisingData()
    );

    gap.setAdvertisingParameters(
      LEGACY_ADVERTISING_HANDLE,
      AdvertisingParameters(
          advertising_type_t::CONNECTABLE_UNDIRECTED, 
          adv_interval_t(millisecond_t(1000))
        )
        .setPrimaryInterval(
          conn_interval_t(millisecond_t(15)), 
          conn_interval_t(millisecond_t(50))
        )
        .setOwnAddressType(own_address_type_t::RANDOM)
        .setPhy(phy_t::LE_1M, phy_t::LE_CODED)
    );
  }

  gDevice.startAdvertising();
}

/* -------------------------------------------------------------------------- */

/* Task used by the event thread, bypassing the usual Arduino loop method. */
void loopTask()
{
  // When disconnected : update the builtin LED.
  if (!gDevice.connected) {
    animateLED(LED_BUILTIN, (gDevice.hasError) ? kLedErrorDelayMilliseconds 
                                               : kLedBeaconDelayMilliseconds);
    return;
  }
  analogWrite(LED_BUILTIN, 30);
  
  // Read analog joystick input.
  gJoystick.Read();

  // Post-process inputs.
  float fx = kJoystickSensibility * gJoystick.x();
  float fy = kJoystickSensibility * gJoystick.y();
  auto buttons = gJoystick.button() ? HIDMouseService::BUTTON_LEFT 
                                    : HIDMouseService::BUTTON_NONE;

  // When demo mode is enabled we bypass the captured values to output random noises.
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

/* -------------------------------------------------------------------------- */

void setup()
{
  // General Setup.
  pinMode(LED_BUILTIN, OUTPUT);
  gJoystick.Calibrate();

  // Initialize the BLE device.
  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(bleScheduleEventsProcessing_cb);
  ble.init(bleInitComplete_cb);

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
