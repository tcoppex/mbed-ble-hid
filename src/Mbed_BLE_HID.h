#ifndef MBED_BLE_HID_H_
#define MBED_BLE_HID_H_

#include <memory>

// from the Mbed SDK.
#include <mbed.h>
#include <DeviceInformationService.h>
#include <BatteryService.h>

#include "services/HIDService.h"

/* -------------------------------------------------------------------------- */

/**
* The MbedBleHID class acts as an interface to create Human Interface Device
* using the bluetooth low energy HID over GATT Profile on Mbed stack.
*/
class MbedBleHID : public Gap::EventHandler,
                   public SecurityManager::EventHandler
{
  private:
    static constexpr int kDefaultStringSize = 32;
    static const char kDefaultDeviceName[kDefaultStringSize];
    static const char kDefaultManufacturerName[kDefaultStringSize];
    static const char kDefaultVersionString[kDefaultStringSize];
    static const int  kDefaultBatteryLevel;

  public:
    static void RunEventThread( void (*task_fn)() );

  public:
    MbedBleHID(const char* deviceName = kDefaultDeviceName,
               const char* manufacturerName = kDefaultManufacturerName,
               const char* versionString = kDefaultVersionString)
      : kDeviceName_(deviceName)
      , kManufacturerName_(manufacturerName)
      , kVersionString_(versionString)
    {}

    virtual ~MbedBleHID() {}

    /** Initialize Bluetooth Low Energy */
    void initialize();

    // -- Getters --
    inline bool connected() const { return connected_; }
    inline bool has_error() const { return error_ != BLE_ERROR_NONE; }
    uint64_t connection_time() const;

  protected:
    /** */
    virtual std::shared_ptr<HIDService> CreateHIDService(BLE &ble) = 0;

    /** Setup the bluetooth HID after BLE initialization. */
    void postInitialization(BLE::InitializationCompleteCallbackContext *params);

    /** Make the device available for connection. */
    void startAdvertising();
  
    // -- Gap::EventHandler Callbacks --
    /** Callback when the ble device connect to another device. */
    void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override;

    /** Callback when the ble device disconnected from another device. */
    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override;

    /** Callback when the peer request connection parameters updates. */
    void onUpdateConnectionParametersRequest(const ble::UpdateConnectionParametersRequestEvent &event) override;
    
    /** Callback when connection parameters have been updated. */
    void onConnectionParametersUpdateComplete(const ble::ConnectionParametersUpdateCompleteEvent &event) override {}

    // -- SecurityManager::EventHandler Callbacks --
    void pairingRequest(ble::connection_handle_t connectionHandle) override;
    // void pairingResult(ble::connection_handle_t connectionHandle, SecurityManager::SecurityCompletionStatus_t result) override;

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

    // Last connection time tick.
    uint64_t lastConnection_ = 0uL;

    // Last ble error captured.
    ble_error_t error_       = BLE_ERROR_NONE;
    
    // State of the connection.
    bool connected_          = false;

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
    ~BasicMbedBleHID() override {}

    /** Return a raw pointer to the underlying HIDService for user updates. */
    inline T* hid() { 
      // [ Hacky, a safer approach would require ffti to use dynamic_pointer_cast
      // to return the polymorphic shared_ptr. ] 
      return reinterpret_cast<T *const>(services_.hid.get());
    }

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
