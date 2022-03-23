#include "Mbed_BLE_HID.h"
#include <chrono>

/* -------------------------------------------------------------------------- */

//
// see :
// https://github.com/ARMmbed/mbed-os-example-ble
//

/* -------------------------------------------------------------------------- */

REDIRECT_STDOUT_TO(Serial); //

/* [debug macro] Set to true to return on catched ble errors.  */
#if 1
# define HANDLE_ERROR(x)  if (has_error()) { printf(x " (%d)\n", __LINE__); return; }
# define DEBUG_LOG()  printf("%s %d\n", __FUNCTION__, __LINE__)
#else
# define HANDLE_ERROR(x)
# define DEBUG_LOG()
#endif

/* -------------------------------------------------------------------------- */

namespace {

/* Behavior for security manager setPairingRequestAuthorisation. */
static constexpr bool kSecurity_SetPairingRequestAuthorisation   = true;
static constexpr bool kSecurity_AcceptPairingRequest             = true;

/* Behavior for GAP manageConnectionParametersUpdateRequest. */
static constexpr bool kGAP_ManageConnectionParamsUpdateRequest   = false;
static constexpr bool kGAP_AcceptConnectionParams                = false;

/* Mbed event queue. */
static constexpr int kEventQueueSize = 16 * EVENTS_EVENT_SIZE;
static events::EventQueue eventQueue(kEventQueueSize);

/* BLE events scheduling callback. */
void bleScheduleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) 
{
  BLE &ble = BLE::Instance();
  eventQueue.call(mbed::Callback<void()>(&ble, &BLE::processEvents));
}

} // namespace

/* -------------------------------------------------------------------------- */

const char MbedBleHID::kDefaultDeviceName[]       = "Mbed-BLE-HID";
const char MbedBleHID::kDefaultManufacturerName[] = "Acme Interactive";
const char MbedBleHID::kDefaultVersionString[]    = "1.4.0";
const int MbedBleHID::kDefaultBatteryLevel        = 98;

/* -------------------------------------------------------------------------- */

void MbedBleHID::RunEventThread(mbed::Callback<void()> task_cb)
{
  static_assert( MBED_MAJOR_VERSION == 6 );

  // Transform the Arduino loop into an update event task.
  const auto event_delay = std::chrono::milliseconds(kDefaultEventQueueDelayMilliseconds);
  eventQueue.call_every(event_delay, task_cb);

  // Launch a new thread for handling events.
#if 0
  rtos::Thread eventThread;
  eventThread.start(mbed::callback(&eventQueue, &events::EventQueue::dispatch_forever));

  // Put the main thread to sleep.
  const auto wait_delay = std::chrono::milliseconds(osWaitForever);
  rtos::ThisThread::sleep_for(wait_delay); //
#else
  eventQueue.dispatch_forever();
#endif
}

/* -------------------------------------------------------------------------- */

void MbedBleHID::initialize()
{
  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(bleScheduleEventsProcessing);
  ble.init(this, &MbedBleHID::postInitialization);
}

uint32_t MbedBleHID::connection_time() const 
{
  return millis() - lastConnection_;
}

void MbedBleHID::postInitialization(BLE::InitializationCompleteCallbackContext *params)
{
  BLE &ble = params->ble;

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
    services_.battery = std::make_unique<BatteryService>(ble, kDefaultBatteryLevel);
    services_.hid = CreateHIDService(ble);
  }

  // Security Manager.
  //
  // @see "HID Over GATT Profile specification", section 5.1.2,
  //      "Connection Procedure for Non-bonded Devices" (p25). 
  //
  // @see https://os.mbed.com/docs/mbed-os/v6.15/mbed-os-api-doxy/classble_1_1_security_manager.html
  //      https://os.mbed.com/docs/mbed-os/v6.15/apis/securitymanager.html
  //
  {
    auto &securityManager = ble.securityManager();

    // Initialized Security manager with no Man-in-the-middle (MITM) protection.
    // 
    //  * Bonding requires non volatile memory.
    //  * MITM protection requires IO capabilities.
    //
    error_ = securityManager.init(
        true                           // enable bonding ?
      , false                           // enable MITM protection ?
      , SecurityManager::IO_CAPS_NONE   // security IO capabilities.
      , nullptr                         // passkey.
      , false                           // enable signing ?
      , nullptr                         // dbFilepath.
    );
    HANDLE_ERROR();

    // Disable keypress notifications during passkey entry.
    error_ = securityManager.setKeypressNotification(false);
    HANDLE_ERROR();

    // Request that the stack attempts to save bonding info at initialization or not [require a filesystem].
    // Persistence may fail if abnormally terminated.
    error_ = securityManager.preserveBondingStateOnReset(true); //
    HANDLE_ERROR();

    // Allow the use of legacy pairing when each side doesn't support secure connections.
    error_ = securityManager.allowLegacyPairing(true);
    HANDLE_ERROR();

    // Tell the stack the app needs to authorize pairing request via callbacks.
    error_ = securityManager.setPairingRequestAuthorisation(kSecurity_SetPairingRequestAuthorisation);
    HANDLE_ERROR();

    // Add events callbacks for pairing requests.
    securityManager.setSecurityManagerEventHandler(this);

    securityManager.setHintFutureRoleReversal(false);

    // securityManager.onShutdown(..);
  }

  // GAP Advertising parameters.
  {
    Gap &gap = ble.gap();

    // GAP events callbacks.
    gap.setEventHandler(this);

    using namespace ble;
    
    error_ = gap.setAdvertisingParameters(
      LEGACY_ADVERTISING_HANDLE,
      AdvertisingParameters()
        .setType(advertising_type_t::CONNECTABLE_UNDIRECTED)
        .setPrimaryInterval(
          adv_interval_t(millisecond_t(kMinGapAdvertisingInterval)), //
          adv_interval_t(millisecond_t(kMaxGapAdvertisingInterval))  //
        )
        .setUseLegacyPDU(true)
        .setOwnAddressType(own_address_type_t::RANDOM)
        // .setOwnAddressType(own_address_type_t::PUBLIC)
        .setPhy(
          phy_t::LE_1M,       // preferred TX modulation.
          phy_t::LE_CODED     // preferred RX modulation.
        )
    );
    HANDLE_ERROR();

    error_ = gap.setAdvertisingPayload(
      LEGACY_ADVERTISING_HANDLE,
      AdvertisingDataSimpleBuilder<LEGACY_ADVERTISING_MAX_SIZE>()
        .setFlags( 
           adv_data_flags_t::LE_GENERAL_DISCOVERABLE   // Device is discoverable at any moment. 
         | adv_data_flags_t::BREDR_NOT_SUPPORTED       // Device is LE only. 
        )
        .setName(kDeviceName_.c_str(), true)
        .setAppearance(services_.hid->appearance())
        .setLocalService(GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE)
        .getAdvertisingData()
    );
    HANDLE_ERROR();

    // Scan parameters.
    error_ = gap.setScanParameters(
      ScanParameters()
        .setPhys(true, true) //
        .set1mPhyConfiguration(ble::scan_interval_t(100), ble::scan_window_t(40), false) //
        .setCodedPhyConfiguration(ble::scan_interval_t(80), ble::scan_window_t(60), false) //
    );
    HANDLE_ERROR();

    // Allows the application to accept or reject a connection parameters update request.
    error_ = gap.manageConnectionParametersUpdateRequest(kGAP_ManageConnectionParamsUpdateRequest); //
    HANDLE_ERROR();

    // Don't allow privacy mode for this device.
    error_ = gap.enablePrivacy(false);
    HANDLE_ERROR();
  }

  startAdvertising();
}

void MbedBleHID::startAdvertising()
{
  DEBUG_LOG();

  BLE &ble = BLE::Instance();

  error_ = ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
  HANDLE_ERROR();
}

/* -------------------------------------------------------------------------- */

/* Generic Access Profile (GAP) Event Callbacks */

void MbedBleHID::onAdvertisingReport(const ble::AdvertisingReportEvent &event)
{
  DEBUG_LOG();

  BLE &ble = BLE::Instance();
 
  error_ = ble.gap().connect(
    event.getPeerAddressType(),
    event.getPeerAddress(),
    ble::ConnectionParameters()
  );
  HANDLE_ERROR();
}

void MbedBleHID::onAdvertisingStart(const ble::AdvertisingStartEvent &event)
{
  DEBUG_LOG();
}

void MbedBleHID::onScanTimeout(const ble::ScanTimeoutEvent& event)
{
  DEBUG_LOG();
}

void MbedBleHID::onConnectionComplete(const ble::ConnectionCompleteEvent &event)
{
  DEBUG_LOG();

  // Check error on connection.
  error_ = event.getStatus();
  HANDLE_ERROR();

  // Connection initiated by the device.
#if 1
  bool const bUseSecureLink(true);
  
  BLE &ble = BLE::Instance();
  auto &securityManager = ble.securityManager();
  auto handle = event.getConnectionHandle();

  // Trigger pairing manually.
  if (bUseSecureLink) {
    // Set security to require encryption without MITM protection.
    error_ = securityManager.setLinkSecurity(handle, SecurityManager::SECURITY_MODE_ENCRYPTION_NO_MITM);
  } else {
    error_ = securityManager.requestPairing(handle);
  }
#endif

  // Set connection status.
  // connected_ = !has_error() && (BLE_ERROR_NONE == event.getStatus());
  // if (connected_) {
  //   lastConnection_ = millis();
  //   printf("\tlast connection : %u\n", lastConnection_);
  // } else {
  //   printf("\tConnection fails.");
  // }
  if (has_error()) {
    printf("\tConnection fails.");
  }
}

void MbedBleHID::onAdvertisingEnd(const ble::AdvertisingEndEvent &event)
{
  DEBUG_LOG();
}


void MbedBleHID::onUpdateConnectionParametersRequest(const ble::UpdateConnectionParametersRequestEvent &event)
{
  DEBUG_LOG();

  auto &gap = BLE::Instance().gap();

  if (kGAP_AcceptConnectionParams) {
    gap.acceptConnectionParametersUpdate(
      event.getConnectionHandle(),
      event.getMinConnectionInterval(), 
      event.getMaxConnectionInterval(),
      event.getSlaveLatency(),
      event.getSupervisionTimeout()
    );
    printf("\tAccept.\n");
  } else {
    gap.rejectConnectionParametersUpdate(
      event.getConnectionHandle()
    );
    printf("\tRefuse.\n");
  }
}

void MbedBleHID::onConnectionParametersUpdateComplete(const ble::ConnectionParametersUpdateCompleteEvent &event) 
{
  DEBUG_LOG();

  // NOTE
  // the device is *truly* connected on the second call to this function
  // after connectionCompleted and advertisingEnd.
  if (preConnected_ && !connected_) {
    connected_ = !has_error();
    if (connected_) {
      lastConnection_ = millis();
      printf("\tlast connection : %u\n", lastConnection_);
    }
  } else {
    preConnected_ = true;
  }
}

void MbedBleHID::onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
{
  DEBUG_LOG();

  error_     = BLE_ERROR_NONE;
  preConnected_ = connected_ = false;
  startAdvertising();
}

/* -------------------------------------------------------------------------- */

/* Security Manager Event Callbacks */

void MbedBleHID::pairingRequest(ble::connection_handle_t connectionHandle)
{
  DEBUG_LOG();

  auto &sm = BLE::Instance().securityManager();

  // Disable generation and exchange of signing keys for this connection.
  error_ = sm.enableSigning(connectionHandle, false); //
  HANDLE_ERROR();

  // Disable using Out Of Band data.
  error_ = sm.setOOBDataUsage(connectionHandle, false, false);
  HANDLE_ERROR();

  if (kSecurity_AcceptPairingRequest) {
    sm.acceptPairingRequest(connectionHandle);
    printf("\tAccept.\n");
  } else {
    sm.cancelPairingRequest(connectionHandle);  
    printf("\tRefuse.\n");
  }
}

/* -------------------------------------------------------------------------- */
