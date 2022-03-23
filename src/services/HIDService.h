#ifndef MBED_BLE_HID_SERVICES_HID_SERVICE_H_
#define MBED_BLE_HID_SERVICES_HID_SERVICE_H_

#if BLE_FEATURE_GATT_SERVER

#include <platform/mbed_assert.h>
#include <ble/BLE.h>
#include <USBHID_Types.h>

/* -------------------------------------------------------------------------- */

/* Main types of Application Usage define in a report map 
 (see USB HID Usage table documentation) */
enum HIDApplicationUsage {
  USAGE_UNDEFINED = 0x00,
  USAGE_POINTER   = 0x01,
  USAGE_MOUSE     = 0x02,
  USAGE_RESERVED  = 0x03,
  USAGE_JOYSTICK  = 0x04,
  USAGE_GAMEPAD   = 0x05,
  USAGE_KEYBOARD  = 0x06,
  USAGE_KEYPAD    = 0x07,
  USAGE_MULTIAXIS = 0x08,
};

/* -------------------------------------------------------------------------- */

#ifndef ATT_UUID_HID_REPORT_ID_MAPPING
#define ATT_UUID_HID_REPORT_ID_MAPPING       0x2908
#endif

enum ReportType {
  INPUT_REPORT    = 0x01,
  OUTPUT_REPORT   = 0x02,
  FEATURE_REPORT  = 0x03,
};

struct report_reference_t {
  uint8_t ID;
  uint8_t type;
};

typedef uint8_t *const report_map_t;
typedef uint8_t *const report_t;
typedef GattAttribute* *const report_ref_desc_array_t;

/* -------------------------------------------------------------------------- */

enum ProtocolMode {
  BOOT_PROTOCOL   = 0x00,
  REPORT_PROTOCOL = 0x01,
};

struct hid_information_t {
  uint16_t  bcd_hid        = HID_VERSION_1_11;   // USB HID spec version.
  uint8_t   b_country_code = 0x00;               // Localization used (none).
  uint8_t   flags          = 0x01 | 0x02;        // Enable remote wake & normally connectable.
};

/* -------------------------------------------------------------------------- */

/**
 * BLE Human Interface Device service.
 *
 * @par usage
 *
 * When this class is instantiated, it adds a human interface device service in 
 * the GattServer.
 *
 * @note You can find specification of the human interface device service here:
 * https://www.bluetooth.com/specifications/gatt
 *
 * @attention Multiple instances of this hid service are not supported.
 */
class HIDService {
 public:
  enum HIDType {
    HID_OTHER    = 0,
    HID_KEYBOARD = 1 << 0,
    HID_MOUSE    = 1 << 1,
  };

   /**
   * Instantiate a Human Device Interface service.
   *
   * The construction of a HIDService adds a GATT human interface device service in @p
   * _ble GattServer.
   *
   * @param[in] _ble BLE device which will host the HID service.
   * @param[in] type Specify if the device is to be treat as a mouse, a keyboard, or both.
   *
   */
  HIDService(BLE &_ble,

             HIDType type,

             report_map_t reportMap,
             uint8_t reportMapLength,
             
             report_t inputReport,
             uint8_t inputReportLength,
             report_ref_desc_array_t inputRefDescs,
             uint8_t inputRefDescsLength,

             report_t outputReport = nullptr,
             uint8_t outputReportLength = 0,
             report_ref_desc_array_t outputRefDescs = nullptr,
             uint8_t outputRefDescsLength = 0,

             report_t featureReport = nullptr,
             uint8_t featureReportLength = 0,
             report_ref_desc_array_t featureRefDescs = nullptr,
             uint8_t featureRefDescsLength = 0)
    : ble(_ble)

    , type(type)

    , inputReport(inputReport)
    , inputReportLength(inputReportLength)

    , outputReport(outputReport)
    , outputReportLength(outputReportLength)

    , featureReport(featureReport)
    , featureReportLength(featureReportLength)

    , protocolMode(REPORT_PROTOCOL)

    , inputReportChar(
        GattCharacteristic::UUID_REPORT_CHAR,
        inputReport, inputReportLength, inputReportLength, 
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
      | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE
      | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
        inputRefDescs, inputRefDescsLength
    )

    , outputReportChar(
        GattCharacteristic::UUID_REPORT_CHAR,
        outputReport, outputReportLength, outputReportLength, 
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
      | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE
      | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE,
        outputRefDescs, outputRefDescsLength
    )

    , featureReportChar(
        GattCharacteristic::UUID_REPORT_CHAR,
        featureReport,featureReportLength, featureReportLength, 
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
      | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE,
        featureRefDescs, featureRefDescsLength
    )
    
    , protocolModeChar(
        GattCharacteristic::UUID_PROTOCOL_MODE_CHAR, 
        &protocolMode, 1, 1,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
      | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE
    )

    , reportMapChar(
        GattCharacteristic::UUID_REPORT_MAP_CHAR,
        reportMap, reportMapLength, reportMapLength,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
    )
    , hidInformationChar(
        GattCharacteristic::UUID_HID_INFORMATION_CHAR,
        (uint8_t*)&hidInfo, sizeof(hidInfo), sizeof(hidInfo),
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
    )
    , hidControlPointChar(
        GattCharacteristic::UUID_HID_CONTROL_POINT_CHAR,
        &hidControlPoint, 1, 1,
        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE
    )
  {
    const int kMaxNumCharacteristics = 10;
    GattCharacteristic *characteristics[kMaxNumCharacteristics]{};
    int charindex = 0;

    // Basic security required for Reports, ProtocolMode and Report Map.
    const auto req = GattCharacteristic::SecurityRequirement_t::UNAUTHENTICATED;

    // HID Information.
    characteristics[charindex++] = &hidInformationChar;
    hidInformationChar.setReadSecurityRequirement(req); //

    // HID Control Point.
    characteristics[charindex++] = &hidControlPointChar;
    hidControlPointChar.setWriteSecurityRequirement(req); //

    // Report Map.
    characteristics[charindex++] = &reportMapChar;
    reportMapChar.setReadSecurityRequirement(req);

    
    // Input report.
    if (inputReport) {
      characteristics[charindex++] = &inputReportChar;
      inputReportChar.setReadSecurityRequirement(req);
      inputReportChar.setWriteSecurityRequirement(req);
      inputReportChar.setUpdateSecurityRequirement(req); //
    }
    // Output report.
    if (outputReport) {
      characteristics[charindex++] = &outputReportChar;
      outputReportChar.setReadSecurityRequirement(req);
      outputReportChar.setWriteSecurityRequirement(req);
      outputReportChar.setUpdateSecurityRequirement(req); //
    }
    // Feature report.
    if (featureReport) {
      characteristics[charindex++] = &featureReportChar;
      featureReportChar.setReadSecurityRequirement(req);
      featureReportChar.setWriteSecurityRequirement(req);
      featureReportChar.setUpdateSecurityRequirement(req); //
    }

    // [optional] Protocol Mode.
    if ((type & HID_MOUSE) || (type & HID_KEYBOARD)) {
      characteristics[charindex++] = &protocolModeChar;
      protocolModeChar.setReadSecurityRequirement(req);
      protocolModeChar.setWriteSecurityRequirement(req);
    }

    // [optional] Boot Keyboard report.
    if (type & HID_KEYBOARD) {
      // todo : input/output 
    }
    // [optional] Boot Mouse report.
    if (type & HID_MOUSE) {
      // todo : input
    }

    // Sanity check for overflow.
    MBED_ASSERT(charindex <= kMaxNumCharacteristics);

    // Create the BLE HID Service.
    GattService hidService(
      GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE,
      characteristics, charindex
    );
    ble.gattServer().addService(hidService);
  }

  virtual ~HIDService() = default;

  /** Defines how the device will appeared in bluetooth managers. */
  virtual ble::adv_data_appearance_t appearance() const = 0; 

  /** Send a full report to the distant ble server. */
  void sendReport()
  {
    ble.gattServer().write(
      inputReportChar.getValueHandle(),
      reinterpret_cast<uint8_t*>(inputReport),
      inputReportLength
    );
  }

 protected:
  BLE &ble;

  HIDType type;

  // -- Attributes --
  
  report_t          inputReport;
  uint8_t           inputReportLength;
  
  report_t          outputReport;
  uint8_t           outputReportLength;
  
  report_t          featureReport;
  uint8_t           featureReportLength;

  uint8_t           protocolMode;

  hid_information_t hidInfo;
  uint8_t           hidControlPoint;


  // -- BLE Characteristics --

  // Reports (if any).
  GattCharacteristic inputReportChar;
  GattCharacteristic outputReportChar;
  GattCharacteristic featureReportChar;

  // Optionals (if mouse or keyboard).
  GattCharacteristic protocolModeChar;
  // [todo] input/output keyboard boot report
  // [todo] input mouse boot report

  // Required.
  GattCharacteristic reportMapChar;
  GattCharacteristic hidInformationChar;
  GattCharacteristic hidControlPointChar;
};

/* -------------------------------------------------------------------------- */

#endif // BLE_FEATURE_GATT_SERVER

#endif // MBED_BLE_HID_SERVICES_HID_SERVICE_H_
