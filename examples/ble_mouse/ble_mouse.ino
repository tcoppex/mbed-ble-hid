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

//#define ENABLE_RANDOM_INPUT

/* -------------------------------------------------------------------------- */

static const int kEventQueueSize = 16 * EVENTS_EVENT_SIZE;
static events::EventQueue eventQueue(kEventQueueSize);

AnalogJoystick ajoy(A7, A6, 2);

/* -------------------------------------------------------------------------- */

static const char kDeviceName[]           = "nano33BLE HID";
static const char kManufacturerName[]     = "Acme Interactive";
static const char kGenericVersionString[] = "1234";
static const int  kDefaultBatteryLevel    = 98;

struct ble_hid_t : Gap::EventHandler 
{
  // The HID-over-GATT Profile (HOGP) needs those three services
  // for the device to be recognized as an HID.
  struct {
    DeviceInformationService *deviceInformation;
    BatteryService *battery;
    HIDService *hid;
  } services;

  bool connected = false;


  static void StartAdvertising() {
    BLE::Instance().gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
  }

  void initialize(BLE &ble)
  {
    // Initialized Security manager with no Man-in-the-middle (MITM) protection.
    ble.securityManager().init(
      true,     // enable bonding.
      false,    // disable MITM protection.
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
        AdvertisingParameters(advertising_type_t::CONNECTABLE_UNDIRECTED, true)
          .setPrimaryInterval(conn_interval_t(millisecond_t(30)), conn_interval_t(millisecond_t(50)))
          .setOwnAddressType(own_address_type_t::RANDOM)
          .setPhy(phy_t::LE_1M, phy_t::LE_CODED)
      );
    }
  }

  // -- EventHandler callbacks

  void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
  {
    connected = true;
    analogWrite(LED_BUILTIN, 30);
  }

  void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
  {
    connected = false;
    StartAdvertising();
  }
} gDevice;

/* -------------------------------------------------------------------------- */

void bleScheduleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) 
{
  BLE &ble = BLE::Instance();
  eventQueue.call(mbed::Callback<void()>(&ble, &BLE::processEvents));
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
  if (params->error != BLE_ERROR_NONE) {
    return;
  }
 
  BLE &ble = params->ble;  
  if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
    return;
  }

  gDevice.initialize(ble);
  gDevice.StartAdvertising();
}

/* -------------------------------------------------------------------------- */

void connectionUpdate() 
{
  // Capture analog joystick input.
  ajoy.Read();

  // Post-process inputs.
  const float sensitivity = 0.09f;
  float fx = sensitivity * ajoy.x();
  float fy = sensitivity * ajoy.y();
  auto buttons = ajoy.button() ? HIDMouseService::BUTTON_LEFT 
                               : HIDMouseService::BUTTON_NONE;

#ifdef ENABLE_RANDOM_INPUT
  fx = random(-sensitivity, +sensitivity);
  fy = random(-sensitivity, +sensitivity);
  buttons = HIDMouseService::BUTTON_NONE;
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
  pinMode(LED_BUILTIN, OUTPUT);

  ajoy.Calibrate();

  // Initialize the BLE device.
  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(bleScheduleEventsProcessing);
  ble.init(bleInitComplete);

  // Transform the Arduino loop to an update event call.
  eventQueue.call_every(10, loop);

  // Launch a new thread for handling events.
  rtos::Thread eventThread;
  eventThread.start(mbed::callback(&eventQueue, &events::EventQueue::dispatch_forever));

  // Put the main thread to sleep.
  rtos::ThisThread::sleep_for(osWaitForever);
}

void loop()
{
  if (gDevice.connected) {
    connectionUpdate();
  } else {
    // Animate the main LED while searching for a connection.
    animateLED(LED_BUILTIN, 750);
  }
}

/* -------------------------------------------------------------------------- */
