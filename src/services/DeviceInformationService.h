#ifndef MBED_BLE_HID_SERVICES_DEVICE_INFORMATION_SERVICE_H_
#define MBED_BLE_HID_SERVICES_DEVICE_INFORMATION_SERVICE_H_

#include <ble/BLE.h>
#include <ble/Gap.h>
#include <ble/GattServer.h>

#if BLE_FEATURE_GATT_SERVER

/* -------------------------------------------------------------------------- */

//   GattService::UUID_DEVICE_INFORMATION_SERVICE,
class DeviceInformationService {
 public:
  DeviceInformationService(BLE &_ble,
                           const char *manufacturersName = nullptr,
                           const char *modelNumber       = nullptr,
                           const char *serialNumber      = nullptr,
                           const char *hardwareRevision  = nullptr,
                           const char *firmwareRevision  = nullptr,
                           const char *softwareRevision  = nullptr) :
    ble(_ble),

    manufacturersNameStringCharacteristic(GattCharacteristic::UUID_MANUFACTURER_NAME_STRING_CHAR,
                                          (uint8_t *)manufacturersName,
                                          (manufacturersName != nullptr) ? strlen(manufacturersName) : 0,
                                          (manufacturersName != nullptr) ? strlen(manufacturersName) : 0,
                                          GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ),

    modelNumberStringCharacteristic(GattCharacteristic::UUID_MODEL_NUMBER_STRING_CHAR,
                                    (uint8_t *)modelNumber,
                                    (modelNumber != nullptr) ? strlen(modelNumber) : 0,
                                    (modelNumber != nullptr) ? strlen(modelNumber) : 0,
                                    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ),

    serialNumberStringCharacteristic(GattCharacteristic::UUID_SERIAL_NUMBER_STRING_CHAR,
                                     (uint8_t *)serialNumber,
                                     (serialNumber != nullptr) ? strlen(serialNumber) : 0,
                                     (serialNumber != nullptr) ? strlen(serialNumber) : 0,
                                     GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ),

    hardwareRevisionStringCharacteristic(GattCharacteristic::UUID_HARDWARE_REVISION_STRING_CHAR,
                                         (uint8_t *)hardwareRevision,
                                         (hardwareRevision != nullptr) ? strlen(hardwareRevision) : 0,
                                         (hardwareRevision != nullptr) ? strlen(hardwareRevision) : 0,
                                         GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ),

    firmwareRevisionStringCharacteristic(GattCharacteristic::UUID_FIRMWARE_REVISION_STRING_CHAR,
                                         (uint8_t *)firmwareRevision,
                                         (firmwareRevision != nullptr) ? strlen(firmwareRevision) : 0,
                                         (firmwareRevision != nullptr) ? strlen(firmwareRevision) : 0,
                                         GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ),
    
    softwareRevisionStringCharacteristic(GattCharacteristic::UUID_SOFTWARE_REVISION_STRING_CHAR,
                                         (uint8_t *)softwareRevision,
                                         (softwareRevision != nullptr) ? strlen(softwareRevision) : 0,
                                         (softwareRevision != nullptr) ? strlen(softwareRevision) : 0,
                                         GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ)
  {
    static bool serviceAdded = false; /* We only add the information service once. */
    
    if (serviceAdded) {
      return;
    }

    GattCharacteristic *charTable[]{ 
      &manufacturersNameStringCharacteristic,
      &modelNumberStringCharacteristic,
      &serialNumberStringCharacteristic,
      &hardwareRevisionStringCharacteristic,
      &firmwareRevisionStringCharacteristic,
      &softwareRevisionStringCharacteristic
    };
    
    GattService deviceInformationService(
      GattService::UUID_DEVICE_INFORMATION_SERVICE, 
      charTable,
      sizeof(charTable) / sizeof(charTable[0])
    );

    ble.gattServer().addService(deviceInformationService);
    serviceAdded = true;
  }

 protected:
  BLE                &ble;

  GattCharacteristic  manufacturersNameStringCharacteristic;
  GattCharacteristic  modelNumberStringCharacteristic;
  GattCharacteristic  serialNumberStringCharacteristic;
  GattCharacteristic  hardwareRevisionStringCharacteristic;
  GattCharacteristic  firmwareRevisionStringCharacteristic;
  GattCharacteristic  softwareRevisionStringCharacteristic;
};

/* -------------------------------------------------------------------------- */

#endif // BLE_FEATURE_GATT_SERVER

#endif 
