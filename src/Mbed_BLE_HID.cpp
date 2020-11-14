
#include "Mbed_BLE_HID.h"

/* -------------------------------------------------------------------------- */

namespace {

// Mbed event queue.
static const int kEventQueueSize = 16 * EVENTS_EVENT_SIZE;
static events::EventQueue eventQueue(kEventQueueSize);

// BLE events scheduling callback.
void bleScheduleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) 
{
  BLE &ble = BLE::Instance();
  eventQueue.call(mbed::Callback<void()>(&ble, &BLE::processEvents));
}

// Redefine Arduino millis() method for MbedOS.
#ifndef millis
uint32_t millis() {
  static mbed::Timer sTimer;
  static bool bInit(false);
  if (!bInit) {
    sTimer.start();
    bInit = true;
  } 
  return sTimer.read_ms();
}
#endif

} // namespace

/* -------------------------------------------------------------------------- */

const char MbedBleHID::kDefaultDeviceName[]       = "Mbed-BLE-HID";
const char MbedBleHID::kDefaultManufacturerName[] = "Acme Interactive";
const char MbedBleHID::kDefaultVersionString[]    = "1234";
const int MbedBleHID::kDefaultBatteryLevel        = 98;

/* -------------------------------------------------------------------------- */

void MbedBleHID::RunEventThread( void (*task_fn)() )
{
  // Transform the Arduino loop into an update event task.
  eventQueue.call_every(10, task_fn);

  // Launch a new thread for handling events.
  rtos::Thread eventThread;
  eventThread.start(mbed::callback(&eventQueue, &events::EventQueue::dispatch_forever));

  // Put the main thread to sleep.
  rtos::ThisThread::sleep_for(osWaitForever);
}

/* -------------------------------------------------------------------------- */

void MbedBleHID::initialize()
{
  static MbedBleHID &HigherSelf = *this; // [an hack transcending reality itself]

  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(bleScheduleEventsProcessing);
  ble.init( [](auto *params) {
    HigherSelf.postInitialization(params->ble);
  });
}

unsigned long MbedBleHID::connection_time() const {
  return millis() - lastConnection_;
}

void MbedBleHID::postInitialization(BLE &ble)
{
  if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
    hasError_ = true;
    return;
  }

  // Add the required BLE services for the HID-over-GATT Profile.
  services_.deviceInformation = std::make_unique<DeviceInformationService>(ble,
    kManufacturerName_.c_str(),
    kVersionString_.c_str(),    // Model Number
    kVersionString_.c_str(),    // Serial Number
    kVersionString_.c_str(),    // Hardware Revision
    kVersionString_.c_str(),    // Firmware Revision
    kVersionString_.c_str()     // Software Revision
  );
  services_.battery = std::make_unique<BatteryService>(ble, kDefaultBatteryLevel); //
  services_.hid = CreateHIDService(ble);

  // Initialized Security manager with no Man-in-the-middle (MITM) protection.
  // @note: When bonding is enabled subsequent pairing after the initial one
  //        fail, which might be a security parameters issue. 
  ble.securityManager().init(
    false,      // disable bonding.
    false,      // disable MITM protection.
    SecurityManager::IO_CAPS_NONE
  );

  // GAP events callbacks.
  Gap &gap = ble.gap();
  gap.setEventHandler(this);

  const Gap::ConnectionParams_t connectionParams = {
    7,          // min conn interval
    15,         // max conn interval
    0,          // slave latency
    3200        // supervision timeout
  };
  gap.setPreferredConnectionParams(&connectionParams);

  // GAP Advertising parameters.
  {
    using namespace ble;

    gap.setAdvertisingPayload(
      LEGACY_ADVERTISING_HANDLE,
      AdvertisingDataSimpleBuilder<LEGACY_ADVERTISING_MAX_SIZE>()
        .setFlags(GapAdvertisingData::BREDR_NOT_SUPPORTED 
                | GapAdvertisingData::LE_GENERAL_DISCOVERABLE
        )
        .setName(kDeviceName_.c_str(), true)
        .setAppearance(services_.hid->appearance())
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

  startAdvertising();
}

void MbedBleHID::startAdvertising()
{
  if (BLE::Instance().gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE)) {
    hasError_ = true;
  }
}

void MbedBleHID::onConnectionComplete(const ble::ConnectionCompleteEvent &event)
{
  hasError_ = (BLE_ERROR_NONE != event.getStatus());
  connected_ = !hasError_;
  if (connected_) {
    lastConnection_ = millis();
  }
}

void MbedBleHID::onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
{
  hasError_  = false;
  connected_ = false;
  startAdvertising();
}

/* -------------------------------------------------------------------------- */
