#ifndef MBED_BLE_HID_H_
#define MBED_BLE_HID_H_

#include <memory>

#include <mbed.h> // Mbed 6.13 SDK

#include "services/DeviceInformationService.h"
#include "services/BatteryService.h"
#include "services/HIDService.h"

/* -------------------------------------------------------------------------- */

/**
* The MbedBleHID class acts as an interface to create Human Interface Device
* using the bluetooth low energy HID over GATT Profile on a Mbed stack.
*/
class MbedBleHID : public Gap::EventHandler,
                   public SecurityManager::EventHandler
{
  private:
    /** Default device's string parameters. */
    static constexpr int kDefaultStringSize = 32;
    static const char kDefaultDeviceName[kDefaultStringSize];
    static const char kDefaultManufacturerName[kDefaultStringSize];
    static const char kDefaultVersionString[kDefaultStringSize];
    static const int  kDefaultBatteryLevel;

    /** Delay between event queue calls. */
    static constexpr int kDefaultEventQueueDelayMilliseconds = 10; // ~ 90fps

    /** Gap advertising interval (30ms to 50ms). */
    static constexpr int kMinGapAdvertisingInterval = 3 * kDefaultEventQueueDelayMilliseconds;
    static constexpr int kMaxGapAdvertisingInterval = 5 * kDefaultEventQueueDelayMilliseconds;

  public:
    /** Launch the event thread. */
    static void RunEventThread(mbed::Callback<void()> task_cb);

  public:
    MbedBleHID(const char* deviceName       = kDefaultDeviceName,
               const char* manufacturerName = kDefaultManufacturerName,
               const char* versionString    = kDefaultVersionString)
      : kDeviceName_(deviceName)
      , kManufacturerName_(manufacturerName)
      , kVersionString_(versionString)
    {}

    virtual ~MbedBleHID() = default;

    /** Initialize Bluetooth Low Energy */
    void initialize();

    /** Return time elapsed from last connection in milliseconds. */
    uint32_t connection_time() const;
    
    /** Return true when an error occurred, false otherwhise. */
    inline bool has_error() const noexcept { return error_ != BLE_ERROR_NONE; }
    
    /** Return connection state. */
    inline bool connected() const noexcept { return connected_; }
    inline bool disconnected() const noexcept { return !connected_; }

  protected:
    /** */
    virtual std::shared_ptr<HIDService> CreateHIDService(BLE &ble) = 0;

    /** Setup the bluetooth HID after BLE initialization. */
    void postInitialization(BLE::InitializationCompleteCallbackContext *params);

    /** Make the device available for connection. */
    void startAdvertising();
  
    // -- Gap::EventHandler Callbacks --
    void onAdvertisingReport(const ble::AdvertisingReportEvent &event) override;
    void onAdvertisingStart(const ble::AdvertisingStartEvent &event) override;
    void onAdvertisingEnd(const ble::AdvertisingEndEvent &event) override;
    void onScanTimeout(const ble::ScanTimeoutEvent& event) override;

    /** Callback when the ble device connect to another device. */
    void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override;

    /** Callback when the ble device disconnected from another device. */
    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override;

    /** Callback when the peer request connection parameters updates. */
    void onUpdateConnectionParametersRequest(const ble::UpdateConnectionParametersRequestEvent &event) override;
    
    /** Callback when connection parameters have been updated. */
    void onConnectionParametersUpdateComplete(const ble::ConnectionParametersUpdateCompleteEvent &event) override;


    // -- SecurityManager::EventHandler Callbacks --

    /** Manually accept or Cancel a pairing request. */
    void pairingRequest(ble::connection_handle_t connectionHandle) override;

  protected:
    const std::string kDeviceName_;
    const std::string kManufacturerName_;
    const std::string kVersionString_;

    // The HID-over-GATT Profile (HOGP) requires at least those three services
    // for the device to be recognized as an HID.
    struct {
      std::unique_ptr<DeviceInformationService> deviceInformation;
      std::unique_ptr<BatteryService> battery;
      std::shared_ptr<HIDService> hid;
    } services_;

    // Last ble error captured.
    ble_error_t error_ = BLE_ERROR_NONE;

    // Last connection time.
    uint32_t lastConnection_ = 0u;
    
    // State of the connection.
    bool connected_ = false;

  private:
    bool preConnected_ = false;
};

/* -------------------------------------------------------------------------- */

/**
* Wrapper around MbedBleHID for simple HIDService with no parameters other than
* the BLE instance in their constructors.
*/
template<typename T>
class BasicMbedBleHID : public MbedBleHID {
  public:
    BasicMbedBleHID(const char* deviceName = kDefaultDeviceName,
                    const char* manufacturerName = kDefaultManufacturerName,
                    const char* versionString = kDefaultVersionString)
      : MbedBleHID(deviceName, manufacturerName, versionString)
    {}

    ~BasicMbedBleHID() override = default;

    /** Return a raw pointer to the underlying HIDService for user updates. */
    inline T* hid() {
      return reinterpret_cast<T *const>(services_.hid.get());
    }

    // [ Safer approach, would requires Run-Time Type Information (RTTI) ]
    /** Return a shared_ptr to the underlying HIDService for user updates. */
    // inline std::shared_ptr<T> hid() {
    //   return std::dynamic_pointer_cast<T>(services_.hid);
    // }

  private:
    std::shared_ptr<HIDService> CreateHIDService(BLE &ble) override {
      return std::static_pointer_cast<HIDService>( std::make_shared<T>(ble) );
    }
};

/* -------------------------------------------------------------------------- */

// Forward declare the Arduino loop function. 
// It will be used as a task by the events thread, bypassing its usual serial call.
void loop(void);
#define MbedBleHID_RunEventThread()  MbedBleHID::RunEventThread(loop)

/* -------------------------------------------------------------------------- */

#endif // MBED_BLE_HID_H_
