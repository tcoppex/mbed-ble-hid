#include "Mbed_BLE_HID.h"
#include <chrono>

/* -------------------------------------------------------------------------- */

REDIRECT_STDOUT_TO(Serial); //

/* [debug macro] Set to true to return on catched ble errors.  */
#if !defined(NDEBUG)
# define HANDLE_ERROR(x)  if (has_error()) { printf(x " (%d)\n", __LINE__); return; }
# define DEBUG_LOG()  printf("%s %d\n", __FUNCTION__, __LINE__)
#else
# define HANDLE_ERROR(x)
# define DEBUG_LOG()
#endif

/* -------------------------------------------------------------------------- */

namespace {

/* Behavior for security manager setPairingRequestAuthorisation. */
// static constexpr bool kSecurity_SetPairingRequestAuthorisation   = true;
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

  if (params->error != BLE_ERROR_NONE) {
    fprintf(stderr, "[BLE] Initialization failed: %d\n", params->error);
    return;
  }

  // Services.
  {
    // Add the required BLE services for the HID-over-GATT Profile.
    services_.deviceInformation = std::make_unique<DeviceInformationService>(
      ble,
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
    if (has_error()) {
      fprintf(stderr, "[BLE Warning] Security init issue: %d\n", error_);
    }

    // Allow the use of legacy pairing when each side doesn't support secure connections.
    error_ = securityManager.allowLegacyPairing(true);
    HANDLE_ERROR();

    // Enable persistent storage across resets.
    error_ = securityManager.preserveBondingStateOnReset(false);
    HANDLE_ERROR();

    // Let Mbed handle auto-accepting bonded peers.
    error_ = securityManager.setPairingRequestAuthorisation(false);
    HANDLE_ERROR();

    // Disable keypress notifications during passkey entry.
    error_ = securityManager.setKeypressNotification(false);
    HANDLE_ERROR();

    // Add events callbacks for pairing requests.
    securityManager.setSecurityManagerEventHandler(this);
    securityManager.setHintFutureRoleReversal(false);
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
        .setPhy(
          phy_t::LE_1M,       // preferred TX modulation.
          phy_t::LE_1M        // preferred RX modulation.
        )
    );
    if (has_error()) {
      fprintf(stderr, "[BLE Error] Advertising parameters setup failed: %d\n", error_);
    }

    // Advertising payload
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
    if (has_error()) {
      fprintf(stderr, "[BLE Error] Advertising payload setup failed: %d\n", error_);
    }

    // Scan parameters.
    // error_ = gap.setScanParameters(
    //   ScanParameters()
    //     .setPhys(true, true) //
    //     .set1mPhyConfiguration(ble::scan_interval_t(100), ble::scan_window_t(40), false) //
    //     .setCodedPhyConfiguration(ble::scan_interval_t(80), ble::scan_window_t(60), false) //
    // );
    // HANDLE_ERROR();

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
  BLE &ble = BLE::Instance();
  error_ = ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
  HANDLE_ERROR();
}

void MbedBleHID::clearSavedBonds()
{
  BLE &ble = BLE::Instance();
  error_ = ble.securityManager().purgeAllBondingState();
  HANDLE_ERROR();
}

/* -------------------------------------------------------------------------- */

/* Generic Access Profile (GAP) Event Callbacks */

void MbedBleHID::onAdvertisingReport(const ble::AdvertisingReportEvent &event)
{
  DEBUG_LOG();
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
  if (has_error()) {
    printf("\tConnection failed with status: %d\n", error_);
    connected_ = false;
    startAdvertising();
    return;
  }

  connected_ = true;
  lastConnection_ = millis();
  printf("\tConnected successfully. Time: %u ms\n", lastConnection_);

  // Request security link encryption
  BLE &ble = BLE::Instance();
  error_ = ble.securityManager().setLinkSecurity(
    event.getConnectionHandle(),
    SecurityManager::SECURITY_MODE_ENCRYPTION_NO_MITM
  );
  if (has_error()) {
    printf("\tFailed to request link security: %d\n", error_);
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
    printf("\tAccepted host connection parameter updates.\n");
  } else {
    gap.rejectConnectionParametersUpdate(event.getConnectionHandle());
    printf("\tRejected host connection parameter updates.\n");
  }
}

void MbedBleHID::onConnectionParametersUpdateComplete(const ble::ConnectionParametersUpdateCompleteEvent &event) 
{
  DEBUG_LOG();
}

void MbedBleHID::onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
{
  DEBUG_LOG();
  printf("\tDisconnected. Reason: %d\n", event.getReason().value());

  error_ = BLE_ERROR_NONE;
  preConnected_ = connected_ = false;

  // Resume advertising so hosts can reconnect
  startAdvertising();
}

/* -------------------------------------------------------------------------- */

/* Security Manager Event Callbacks */

void MbedBleHID::pairingRequest(ble::connection_handle_t connectionHandle)
{
  DEBUG_LOG();

  auto &sm = BLE::Instance().securityManager();

  if (kSecurity_AcceptPairingRequest) {
    sm.acceptPairingRequest(connectionHandle);
    printf("\tAccepted pairing request.\n");
  } else {
    sm.cancelPairingRequest(connectionHandle);
    printf("\tRefused pairing request.\n");
  }
}

void MbedBleHID::pairingResult(
  ble::connection_handle_t connectionHandle,
  ble::SecurityManager::SecurityCompletionStatus_t result
)
{
  DEBUG_LOG();

  if (result == ble::SecurityManager::SEC_STATUS_SUCCESS) {
    printf("\tPairing successfully completed.\n");
  } else {
    printf("\tPairing failed with status error code: 0x%02x\n", result);

    // Disconnect so the central host realizes the keys are invalid,
    // allowing the user to unpair and re-pair cleanly.
    BLE::Instance().gap().disconnect(
      connectionHandle,
      ble::local_disconnection_reason_t::AUTHENTICATION_FAILURE
    );
  }
}

void MbedBleHID::linkEncryptionResult(ble::connection_handle_t connectionHandle, ble::link_encryption_t result)
{
  DEBUG_LOG();

  if (result == ble::link_encryption_t::ENCRYPTED
   || result == ble::link_encryption_t::ENCRYPTED_WITH_MITM) {
    printf("\tLink successfully encrypted.\n");
  } else {
    printf("\tLink encryption failed (missing or stale keys). Rejecting bond.\n");

    clearSavedBonds();

    // Disconnect so the central host realizes the keys are invalid,
    // allowing the user to unpair and re-pair cleanly.
    BLE::Instance().gap().disconnect(
      connectionHandle,
      ble::local_disconnection_reason_t::AUTHENTICATION_FAILURE
    );
  }
}

/* -------------------------------------------------------------------------- */
