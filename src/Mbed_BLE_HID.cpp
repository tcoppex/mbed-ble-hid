
#include "Mbed_BLE_HID.h"

/* -------------------------------------------------------------------------- */
//
// Notes :
//  * On Linux, might have to change /etc/bluetooth/input.conf for HID to work.
//    ( https://askubuntu.com/questions/1065335/bluetooth-mouse-constantly-disconnects-and-reconnects )
//
/* -------------------------------------------------------------------------- */

/* [debug macro] Set to true to return on some errors.  */
#if 0
# define HANDLE_ERROR()  if (error_) { return; }
#else
# define HANDLE_ERROR()
#endif

/* -------------------------------------------------------------------------- */

namespace {

/* Internal generic parameters. */
static constexpr bool bAcceptConnectionParams = true; //
static constexpr bool bAcceptPairingRequest   = true; //

/* Mbed event queue. */
static const int kEventQueueSize = 16 * EVENTS_EVENT_SIZE;
static events::EventQueue eventQueue(kEventQueueSize);

/* BLE events scheduling callback. */
void bleScheduleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) 
{
  BLE &ble = BLE::Instance();
  eventQueue.call(mbed::Callback<void()>(&ble, &BLE::processEvents));
}

/* Redefines Arduino millis() method when on PlatformIO. */
static uint64_t GetElapsedTimeMilliseconds() {
#ifdef PLATFORMIO
  static mbed::Timer sTimer;
  static bool bInit(false);

  if (!bInit) {
    sTimer.start();
    bInit = true;
  } 
  // Mbed OS v6.
  // auto ms = chrono::duration_cast<chrono::milliseconds>(sTimer.elapsed_time()).count();
  // Mbed OS v5 [deprecated].
  return static_cast<uint64_t>(sTimer.read_ms());
#else
  return static_cast<uint64_t>(millis());
#endif
}

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
  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(bleScheduleEventsProcessing);
  ble.init(this, &MbedBleHID::postInitialization);
}

uint64_t MbedBleHID::connection_time() const {
  return GetElapsedTimeMilliseconds() - lastConnection_;
}

void MbedBleHID::postInitialization(BLE::InitializationCompleteCallbackContext *params)
{
  BLE &ble = params->ble;
  
  if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
    return;
  }

  // Services.
  {
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
  }

  // Security Manager.
  // ( https://os.mbed.com/docs/mbed-os/v5.15/apis/securitymanager.html )
  // ( https://os.mbed.com/docs/mbed-os/v5.15/feature-i2c-doxy/class_security_manager.html )
  {
    auto &securityManager = ble.securityManager();

    // Initialized Security manager with no Man-in-the-middle (MITM) protection.
    // @note: When bonding is enabled subsequents pairing after the initial one
    //        might fail, which could be a security parameters issue. 
    error_ = securityManager.init(
      false,                          // enable bonding ?
      false,                          // enable MITM protection ?
      SecurityManager::IO_CAPS_NONE,  // security IO capabilities.
      nullptr,                        // passkey.
      false,                          // enable signing ?
      nullptr                         // dbFilepath.
    );
    HANDLE_ERROR();

    // Request that the stack attempts to save bonding info at initialization.
    error_ = securityManager.preserveBondingStateOnReset(true);
    HANDLE_ERROR();

    // Allow the use of legacy pairing when each side doesn't support secure connections.
    securityManager.allowLegacyPairing(true);

    // Add events callbacks for pairing requests.
    securityManager.setSecurityManagerEventHandler(this);
  }

  // GAP Advertising parameters.
  {
    Gap &gap = ble.gap();
    
    // GAP events callbacks.
    gap.setEventHandler(this);

    // Allows the application to accept or reject a connection parameters update request.
    gap.manageConnectionParametersUpdateRequest(true); //

    using namespace ble;
    gap.setAdvertisingPayload(
      LEGACY_ADVERTISING_HANDLE,
      AdvertisingDataSimpleBuilder<LEGACY_ADVERTISING_MAX_SIZE>()
        .setFlags(adv_data_flags_t::BREDR_NOT_SUPPORTED       // Peripheral device is LE only. 
                | adv_data_flags_t::LE_GENERAL_DISCOVERABLE   // Peripheral device is discoverable at any moment. 
        )
        .setName(kDeviceName_.c_str(), true)
        .setAppearance(services_.hid->appearance())
        .setLocalService(GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE)
        .getAdvertisingData()
    );

    gap.setAdvertisingParameters(
      LEGACY_ADVERTISING_HANDLE,
      AdvertisingParameters()
        .setType(advertising_type_t::CONNECTABLE_UNDIRECTED)
        .setPrimaryInterval(
          conn_interval_t(millisecond_t(100)), //
          conn_interval_t(millisecond_t(200))  //
        )
        .setUseLegacyPDU(true)
        .setOwnAddressType(own_address_type_t::RANDOM)
        .setPhy(phy_t::LE_1M, phy_t::LE_CODED)
    );
  }

  startAdvertising();
}

void MbedBleHID::startAdvertising()
{
  BLE &ble = BLE::Instance();

  // Start Advertising.
  error_ = ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
  HANDLE_ERROR();

  // Tell the stack the app needs to authorize pairing request via callbacks.
  error_ = ble.securityManager().setPairingRequestAuthorisation(true); //
  HANDLE_ERROR();
}

void MbedBleHID::onConnectionComplete(const ble::ConnectionCompleteEvent &event)
{
  BLE &ble = BLE::Instance();
  auto &sm = ble.securityManager();
  auto handle = event.getConnectionHandle();

  // Check error on connection.
  error_ = event.getStatus();
  HANDLE_ERROR();

  // ---------------------------------
  // Trigger pairing manually.
  // error_ = sm.requestPairing(handle);

  // (alternative)
  // Set security to require encryption without MITM protection.
  // error_ = sm.setLinkSecurity(
  //   handle,
  //   SecurityManager::SECURITY_MODE_ENCRYPTION_NO_MITM
  // );
  // ---------------------------------

  connected_ = !has_error();
  
  if (connected_) {
    lastConnection_ = GetElapsedTimeMilliseconds();
  }
}

void MbedBleHID::onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
{
  error_     = BLE_ERROR_NONE;
  connected_ = false;
  startAdvertising();
}

void MbedBleHID::onUpdateConnectionParametersRequest(const ble::UpdateConnectionParametersRequestEvent &event)
{
  auto &gap = BLE::Instance().gap();

  if (bAcceptConnectionParams) {
    gap.acceptConnectionParametersUpdate(
      event.getConnectionHandle(),
      event.getMinConnectionInterval(), 
      event.getMaxConnectionInterval(),
      event.getSlaveLatency(),
      event.getSupervisionTimeout()
    );
  } else {
    gap.rejectConnectionParametersUpdate(
      event.getConnectionHandle()
    );
  }
}

void MbedBleHID::pairingRequest(ble::connection_handle_t connectionHandle)
{
  auto &sm = BLE::Instance().securityManager();

  if (bAcceptPairingRequest) {
    sm.acceptPairingRequest(connectionHandle);
  } else {
    sm.cancelPairingRequest(connectionHandle);  
  }
}


/* -------------------------------------------------------------------------- */
