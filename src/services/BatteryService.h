#ifndef MBED_BLE_HID_SERVICES_BATTERY_SERVICE_H_
#define MBED_BLE_HID_SERVICES_BATTERY_SERVICE_H_

#if BLE_FEATURE_GATT_SERVER

#include <platform/mbed_assert.h>
#include <ble/BLE.h>
#include <ble/Gap.h>
#include <ble/GattServer.h>

/* -------------------------------------------------------------------------- */

//   GattService::UUID_BATTERY_SERVICE

class BatteryService {
 public:
  static constexpr int kMaxLevel = 100;

 public:
  BatteryService(BLE &_ble, uint8_t level = kMaxLevel) :
    ble_(_ble),
    batteryLevel_(level),
    characteristic_(
      GattCharacteristic::UUID_BATTERY_LEVEL_CHAR,
      &batteryLevel_,
      GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
    )
  {
    MBED_ASSERT(level <= kMaxLevel);
    
    GattCharacteristic *charTable[]{ &characteristic_ };
    GattService batteryService(
      GattService::UUID_BATTERY_SERVICE,
      charTable,
      sizeof(charTable) / sizeof(charTable[0])
    );

    ble_.gattServer().addService(batteryService);
  }

  void updateBatteryLevel(uint8_t newLevel)
  {
    MBED_ASSERT(newLevel <= kMaxLevel);

    batteryLevel_ = newLevel;
    ble_.gattServer().write(
      characteristic_.getValueHandle(),
      &batteryLevel_,
      1
    );
  }

 protected:
  BLE &ble_;
  uint8_t batteryLevel_;
  ReadOnlyGattCharacteristic<uint8_t> characteristic_;
};

/* -------------------------------------------------------------------------- */

#endif // BLE_FEATURE_GATT_SERVER

#endif // MBED_BLE_HID_SERVICES_BATTERY_SERVICE_H_
