#ifndef MPPTController_h
#define MPPTController_h

#include <Arduino.h>
#include <HardwareSerial.h>
#include <cstdint>

// Operating Modes
enum MPPTMode {
    MODE_POWER_ON = 0x00,
    MODE_START_CHECK = 0x01,
    MODE_NORMAL_RUN = 0x02,
    MODE_FAULT = 0x03,
    MODE_UPDATE = 0x04,
    MODE_SETPAR = 0x05
};

// Charging Modes
enum ChargingMode {
    CHARGE_NO_CHARGE = 0x00,
    CHARGE_CC = 0x01,      // Constant Current
    CHARGE_CV = 0x02,      // Constant Voltage
    CHARGE_FLOAT = 0x03,
    CHARGE_SOFT_START = 0x04
};

// Battery Types
enum BatteryType {
    BATTERY_GEL1 = 0x00, // Gel1
    BATTERY_AGM1 = 0x01, // AGM1
    BATTERY_SEALED1 = 0x02, // Sealed1
    BATTERY_FLOODED1 = 0x03, // Floaded 1
    BATTERY_LITHIUM = 0x04, // Lithium
    BATTERY_GEL = 0x05, // Gel
    BATTERY_AGM = 0x06, // AGM
    BATTERY_SEALED = 0x07, // Sealed
    BATTERY_FLOODED = 0x08, // Floaded
    BATTERY_USER_DEFINE = 0x09, // UserDefinded
};

enum MachineType{
    CHARGE = 0x01,
    INVERTER = 0x02,
    COMBO = 0x03,
};

// Error Flags (bit positions)
enum ErrorFlag {
    ERROR_IN_RELAY = 0,           // Input relay failure
    ERROR_IN_UVP = 1,             // Input voltage low
    ERROR_IN_OVP = 2,             // Input voltage high
    ERROR_BUCK1_OCP = 3,          // Buck1 current high
    ERROR_BUCK2_OCP = 4,          // Buck2 current high
    ERROR_OUT_UVP = 5,            // Output voltage low
    ERROR_OUT_OVP = 6,            // Output voltage high
    ERROR_OUT_RELAY = 7,          // Output relay failure
    ERROR_OUT_SHORT = 8,          // Output circuit short
    ERROR_POLAR = 9,              // Polar connect error
    ERROR_BAT_VOLT = 10,          // Battery voltage error
    ERROR_BUCK1_FAULT = 11,       // Buck1 circuit fault
    ERROR_D1_MOSFET = 12,         // D1 mosfet failure
    ERROR_BUCK2_FAULT = 13,       // Buck2 circuit fault
    ERROR_D2_MOSFET = 14,         // D2 mosfet failure
    ERROR_BAT_OVER_TEMP = 15,     // Battery temperature high
    ERROR_ENVIR_OVER_TEMP = 16,   // Environment temperature high
    ERROR_BUCK_OVER_TEMP = 17,    // Buck temperature high
    ERROR_FAN1 = 18,              // Fan1 failure
    ERROR_FAN2 = 19,              // Fan2 failure
    ERROR_PV_VOLT_SENSOR = 20,    // PV voltage sensor failure
    ERROR_BUS_VOLT_SENSOR = 21,   // BUS voltage sensor failure
    ERROR_BUCK_VOLT_SENSOR = 22,  // Buck voltage sensor failure
    ERROR_OUT_VOLT_SENSOR = 23,   // Output voltage sensor failure
    ERROR_PV_CUR_SENSOR = 24,     // PV current sensor failure
    ERROR_OUT_CUR_SENSOR = 25,    // Output current sensor failure
    ERROR_BAT_OFF = 26,           // Battery turn off failure
    ERROR_EMERGENCY_STOP = 27,    // Emergency stop failure
    ERROR_BUCK_OVP = 29           // Buck voltage high
};

// Main data structure
    struct MPPTMeasurements {
    // Basic measurements (Function 00)
    float pvVoltage;          // PV voltage (V)
    float busVoltage;         // Bus voltage (V)
    float batteryVoltage;     // Battery/Output voltage (V)
    float chargingCurrent;    // Charging current (A)
    float chargingPower;      // Charging power (W)
    float batteryTemp;    // Battery temperature (C)
    float environmentTemp; // Environment temperature (C)
    MPPTMode operatingMode;   // Current operating mode
    ChargingMode chargingMode; // Current charging mode
    uint32_t dayPower;        // Day power output (Wh)
    uint32_t totalPower;      // Total power output (Wh)
    uint16_t errorFlags;      // Error flags (16-bit bitmap)
};

    struct MPPTCal {
    // Calibration data (Function 01)
    int16_t pvVoltOffset;     // PV voltage adjustment (0.1V)
    uint16_t pvVoltRatio;     // PV voltage ratio
    int16_t outVoltOffset;    // Output voltage adjustment (0.1V)
    uint16_t outVoltRatio;    // Output voltage ratio
    int16_t outCurOffset;     // Output current adjustment
    uint32_t outCurRatio;     // Output current ratio
    String serialNum;         // Serial number
    uint8_t machineAddress;   // Modbus address
    BatteryType batteryType;  // Battery type
    String machineModel;      // Machine Address
    String firmware;          // Firmware
    uint16_t maxChargeCurrent; // Maximum charge current (A)
    float buckChargeVoltage;  // Buck charge voltage (V)
    float floatChargeVoltage; // Float charge voltage (V)
};

    struct MPPTInfo{
    // Device info (Function 03)
    MachineType machineType;      // 01=charge, 02=invert, 03=combo
    String serialNumber;
    BatteryType batteryType;  // Battery type
    String machineModel;
    String masterFirmware;
    String slaveFirmware;
    uint16_t firstPowerCodeDate;
};


class MPPTController {
public:
    MPPTController(HardwareSerial& serial, uint8_t add, int rxPin, int txPin);
    ~MPPTController();

    // Initialization
    bool begin(uint32_t baudRate = 9600);
    void setAddress(uint8_t addr);

    // Read functions
    bool readMeasurements();      // Function 00
    bool readCalibration();       // Function 01
    bool readDeviceInfo();        // Function 03
    // bool readMonthlyPower(uint8_t year, uint8_t month, uint32_t* power);
    // bool readDailyPower(uint8_t year, uint8_t month, uint8_t day, uint16_t* power);
    // bool readHourlyPower(uint8_t day, uint8_t hour, uint16_t* power);

    // Write functions
    bool setBatteryType(BatteryType type);
    bool setMaxChargeCurrent(float current);
    bool LoadModeON(bool loadOn);
    bool setChargeVoltages(float buckVoltage, float floatVoltage);
    bool setMachineAddress(uint8_t addr);
    bool clearTotalPower();
    bool emergencyStop();
    bool clearEmergencyStop();
    bool resetToDefaults();

    // Getters
    MPPTMeasurements* getMeasurements();
    MPPTCal* getCalibration();
    MPPTInfo* getDeviceInfo();
    String getErrorString(uint16_t errorFlags);
    String getModeString(MPPTMode mode);
    String getChargeModeString(ChargingMode mode);
    String getBatteryTypeString(BatteryType type);
    String getMachineTypeString(MachineType type);

private:
    HardwareSerial* _serial;
    int _rxPin;
    int _txPin;
    uint8_t _address;
    uint8_t _buffer[128];
    MPPTMeasurements _measurements;
    MPPTCal _calibration;
    MPPTInfo _info;
    unsigned long _lastRead;
    uint16_t _readInterval;

    // Protocol functions
    bool sendCommand(uint8_t controlCode, uint8_t functionCode,
                    uint8_t* data, uint8_t dataLen, uint8_t* response, uint8_t* respLen);
    uint16_t calculateChecksum(uint8_t* data, uint8_t len);
    MPPTMeasurements* parseMeasurementResponse(uint8_t* data, uint8_t len);
    MPPTCal* parseCalibrationResponse(uint8_t* data, uint8_t len);
    MPPTInfo* parseDeviceInfoResponse(uint8_t* data, uint8_t len);

    // Internal helpers
    void clearBuffer();
};

#endif
