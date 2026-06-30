#include "MPPTController.h"
#include <cstdint>

MPPTController::MPPTController(HardwareSerial& serial, uint8_t addr, int rxPin, int txPin)
: _serial(&serial), _rxPin(rxPin), _txPin(txPin) {
    _address = addr;
    memset(&_measurements, 0, sizeof(_measurements));
    memset(&_calibration, 0, sizeof(_calibration));
    memset(&_info, 0, sizeof(_info));
}

MPPTController::~MPPTController() {
    // Cleanup if needed
}

bool MPPTController::begin(uint32_t baudRate) {
    _serial->begin(baudRate, SERIAL_8N1, _rxPin, _txPin);
    delay(100);

    // Test communication with retries (controller may not answer immediately)
    for (uint8_t attempt = 0; attempt < 5; attempt++) {
        if (readMeasurements()) {
        return true;
        }
        delay(200);
    }
    return false;
}

void MPPTController::setAddress(uint8_t addr) {
    _address = addr;
}

uint16_t MPPTController::calculateChecksum(uint8_t* data, uint8_t len) {
    uint16_t sum = 0;
    Serial.printf("Checksum bytes (0-%d): ", len);
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
        Serial.printf("0x%02X + ", data[i]);
    }
    Serial.printf("= 0x%04X\n", sum);
    return sum;
}

bool MPPTController::sendCommand(uint8_t controlCode, uint8_t functionCode,
                                 uint8_t* cmdData, uint8_t dataLen,
                                 uint8_t* response, uint8_t* respLen) {
    uint8_t command[128];
    uint8_t cmdIdx = 0;

    // CRITICAL: Clear the serial buffer BEFORE sending
     while (_serial->available()) {
         _serial->read();
     }

    // Build command frame
    command[cmdIdx++] = 0x55;           // Header
    command[cmdIdx++] = 0xAA;           // Header
    command[cmdIdx++] = _address;       // Address
    command[cmdIdx++] = controlCode;    // Control code
    command[cmdIdx++] = functionCode;   // Function code

    // Data length (2 bytes, little endian)
    command[cmdIdx++] = (dataLen >> 8) & 0xFF;
    command[cmdIdx++] = dataLen & 0xFF;

    // Copy command data
    for (uint8_t i = 0; i < dataLen; i++) {
        command[cmdIdx] = cmdData[i];
        cmdIdx +=1;
    }


    // Calculate checksum (from address to end of data)
    uint16_t checksum = calculateChecksum(&command[0], cmdIdx);
    // Protocol uses little-endian checksum: low byte, then high byte
    command[cmdIdx++] = (checksum >> 8) & 0xFF;
    command[cmdIdx++] = checksum & 0xFF;


    Serial.print("TX: ");
    for (uint8_t i = 0; i < cmdIdx; i++) {
        Serial.printf("0x%02X ", command[i]);
    }
    Serial.println();


    // Send command
    _serial->write(command, cmdIdx);
    _serial->flush();

    // Wait for response
    delay(200);

    // Read response
    *respLen = 0;
    unsigned long startTime = millis();
    while (millis() - startTime < 500) {
        if (_serial->available()) {
        response[(*respLen)++] = _serial->read();
        startTime = millis();  // Reset timeout on each byte
        }
        if (*respLen >= 64) break;
    }

    Serial.print("Rx: ");
    for (uint8_t i = 0; i < *respLen; i++) {
        Serial.printf("0x%02X ", response[i]);
    }
    Serial.println();

    // Validate response
    if (*respLen < 9) return false;
    if (response[0] != 0x55 || response[1] != 0xAA) return false;
    if (response[2] != _address) return false;

    return true;
}

bool MPPTController::readMeasurements() {
    uint8_t response[128];
    uint8_t respLen = 0;

    // Function 00 - No data needed
    if (sendCommand(0x00, 0x00, nullptr, 0, response, &respLen)) {
        return parseMeasurementResponse(response, respLen) != nullptr;
    }
    return false;
}

MPPTMeasurements* MPPTController::parseMeasurementResponse(uint8_t* data, uint8_t len) {
    if (len < 10) return nullptr;

    // Get data length from response
    uint16_t dataLen = (data[5] << 8) | data[6];
    if (dataLen < 20) return nullptr;

    int idx = 7;  // Start of measurement data
    int payloadEnd = idx + dataLen;
    if (payloadEnd > len) payloadEnd = len;

    #define REQUIRE_BYTES(n) if (idx + (n) > payloadEnd) return nullptr

    // PV Voltage (0.1V)
    REQUIRE_BYTES(2);
    uint16_t pvRaw = (data[idx] << 8) | data[idx+1];
    _measurements.pvVoltage = pvRaw * 0.1f;
    idx += 2;

    // Bus Voltage (0.1V)
    REQUIRE_BYTES(2);
    uint16_t busRaw = (data[idx] << 8) | data[idx+1];
    _measurements.busVoltage = busRaw * 0.1f;
    idx += 2;

    // Buck Voltage (skip - not used)
    REQUIRE_BYTES(2);
    idx += 2;

    // Battery Voltage (0.1V)
    REQUIRE_BYTES(2);
    uint16_t battRaw = (data[idx] << 8) | data[idx+1];
    _measurements.batteryVoltage = battRaw * 0.1f;
    idx += 2;

    // Charging Current (0.01A)
    REQUIRE_BYTES(2);
    uint16_t currentRaw = (data[idx] << 8) | data[idx+1];
    _measurements.chargingCurrent = currentRaw * 0.01f;
    idx += 2;

    // Charging Power (0.1W)
    REQUIRE_BYTES(2);
    uint16_t powerRaw = (data[idx] << 8) | data[idx+1];
    _measurements.chargingPower = powerRaw * 0.1f;
    idx += 2;

    // Battery Temperature (0.1°C)
    REQUIRE_BYTES(2);
    uint16_t battTempRaw = (data[idx] << 8) | data[idx+1];
    _measurements.batteryTemp = battTempRaw;
    idx += 2;

    // Skip Buck1 Temperature (2 bytes)
    REQUIRE_BYTES(2);
    idx += 2;

    // Skip Buck2 Temperature (2 bytes)
    REQUIRE_BYTES(2);
    idx += 2;

    // Enviroment Temperature (2 bytes)
    REQUIRE_BYTES(2);
    uint16_t envTempRaw = (data[idx] << 8) | data[idx+1];
    _measurements.environmentTemp = envTempRaw;
    idx += 2;

    // Operating Mode
    REQUIRE_BYTES(2);
    uint16_t modeRaw = (data[idx] << 8) | data[idx+1];
    _measurements.operatingMode = (MPPTMode)(modeRaw & 0xFF);
    idx += 2;

    // Day Power (4 bytes, direct Wh)
    if (idx + 4 <= payloadEnd) {
        _measurements.dayPower = (data[idx] << 24) | (data[idx+1] << 16) |
                        (data[idx+2] << 8) | data[idx+3];
        idx += 4;
    }

    // Total Power (4 bytes, direct Wh)
    if (idx + 4 <= payloadEnd) {
        _measurements.totalPower = (data[idx] << 24) | (data[idx+1] << 16) |
                        (data[idx+2] << 8) | data[idx+3];
        idx += 4;
    }

    // Charging Mode (next 1 bytes)
    if (idx + 1 <= payloadEnd) {
        uint16_t chargeModeRaw = data[idx];
        _measurements.chargingMode = (ChargingMode)(chargeModeRaw & 0xFF);
    }
    idx += 1;

    // Offset (reserved 2 bytes)
    idx += 2;

    // Error Flags (2 bytes)
    if (idx + 2 <= payloadEnd) {
        _measurements.errorFlags = (data[idx] << 8) | data[idx+1];
    }

    #undef REQUIRE_BYTES

    return &_measurements;
}

bool MPPTController::readCalibration() {
    uint8_t response[128];
    uint8_t respLen = 0;

    if (sendCommand(0x00, 0x01, nullptr, 0, response, &respLen)) {
        return parseCalibrationResponse(response, respLen) != nullptr;
    }
    return false;
}

MPPTCal* MPPTController::parseCalibrationResponse(uint8_t* data, uint8_t len) {
    if (len < 20) return nullptr;

    int idx = 7;

    // PV Voltage Offset
    int16_t pvOffsetRaw = (data[idx] << 8) | data[idx+1];
    _calibration.pvVoltOffset = pvOffsetRaw * 0.1f;
    idx += 2;

    // PV Voltage Ratio
    _calibration.pvVoltRatio = (data[idx] << 8) | data[idx+1];
    idx += 2;

    // Skip Bus Voltage stuff (4 bytes)
    idx += 8;

    // Output Voltage Offset
    int16_t outOffsetRaw = (data[idx] << 8) | data[idx+1];
    _calibration.outVoltOffset = outOffsetRaw * 0.1f;
    idx += 2;

    // Output Voltage Ratio
    _calibration.outVoltRatio = (data[idx] << 8) | data[idx+1];
    idx += 2;

    // Output Current Offset
    int16_t curOffsetRaw = (data[idx] << 8) | data[idx+1];
    _calibration.outCurOffset = curOffsetRaw * 0.01f;
    idx += 2;

    // Output Current Ratio (4 bytes)
    _calibration.outCurRatio = (data[idx] << 24) | (data[idx+1] << 16) | (data[idx+2] << 8) | data[idx+3];
    idx += 4;

    // Serial Number (16 bytes ASCII)
    char serial[17] = {0};
    for (int i = 0; i < 16 && idx + i < len; i++) {
        serial[i] = data[idx + i];
    }
    _calibration.serialNum = String(serial);
    idx += 16;

    // Machine Address
    _calibration.machineAddress = data[idx];
    idx += 1;

    // Battery Type
    _calibration.batteryType = (BatteryType)data[idx];
    idx += 1;

    // Machine Model (32 bytes)
    char machine[33] = {0};
    for (int i = 0; i < 32 && idx + i < len; i++) {
        machine[i] = data[idx + i];
    }
    _calibration.machineModel = String(machine);
    idx += 32;

    // Firmware (4 bytes)
    char firmware[5] = {0};
    for (int i = 0; i < 5 && idx + i < len; i++) {
        firmware[i] = data[idx + i];
    }
    _calibration.firmware = String(firmware);
    idx += 4;

    // Max Charge Current
    int16_t maxCurrent= (data[idx] << 8) | data[idx+1];
    _calibration.maxChargeCurrent = maxCurrent * 0.01f;
    idx += 2;

    // Buck Charge Voltage (0.1V)
    uint16_t buckRaw = (data[idx] << 8) | data[idx+1];
    _calibration.buckChargeVoltage = buckRaw * 0.1f;
    idx += 2;

    // Float Charge Voltage (0.1V)
    uint16_t floatRaw = (data[idx] << 8) | data[idx+1];
    _calibration.floatChargeVoltage = floatRaw * 0.1f;

    return &_calibration;
}

bool MPPTController::readDeviceInfo() {
    uint8_t response[128];
    uint8_t respLen = 0;

    if (sendCommand(0x00, 0x03, nullptr, 0, response, &respLen)) {
        return parseDeviceInfoResponse(response, respLen) != nullptr;
    }
    return false;
}

MPPTInfo* MPPTController::parseDeviceInfoResponse(uint8_t* data, uint8_t len) {
    if (len < 10) return nullptr;

    int idx = 7;

    // Machine Type
    _info.machineType = (MachineType)data[idx];
    idx += 1;

    // Serial Number (16 bytes ASCII)
    char serial[17] = {0};
    for (int i = 0; i < 16 && idx + i < len; i++) {
        serial[i] = data[idx + i];
    }
    _info.serialNumber = String(serial);
    idx += 16;

    // Battery Type
    _calibration.batteryType = (BatteryType)data[idx];
    idx += 1;

    // Machine Model (32 bytes ASCII)
    char model[33] = {0};
    for (int i = 0; i < 32 && idx + i < len; i++) {
        model[i] = data[idx + i];
    }
    _info.machineModel = String(model);
    idx += 32;

    // Main Firmware (4 bytes ASCII)
    char mainFw[5] = {0};
    for (int i = 0; i < 4 && idx + i < len; i++) {
        mainFw[i] = data[idx + i];
    }
    _info.masterFirmware = String(mainFw);
    idx += 4;

    // Slave Firmware (4 bytes ASCII)
    char slaveFw[5] = {0};
    for (int i = 0; i < 4 && idx + i < len; i++) {
        slaveFw[i] = data[idx + i];
    }
    _info.slaveFirmware = String(slaveFw);


    return &_info;
}

// Write functions
bool MPPTController::setBatteryType(BatteryType type) {
    uint8_t cmdData[1] = {(uint8_t)type};
    uint8_t response[128];
    uint8_t respLen = 0;

    return sendCommand(0x01, 0x0C, cmdData, 1, response, &respLen);
}

bool MPPTController::setMaxChargeCurrent(float current) { // Unit is Amps
    // Scale: 1 Amp = 100 (0.01A per unit)
    uint16_t scaledValue = (uint16_t)(current * 100.0f);

    // Data: High byte first, then low byte
    uint8_t cmdData[2] = {
        (uint8_t)((scaledValue >> 8) & 0xFF),  // High byte
        (uint8_t)(scaledValue & 0xFF)          // Low byte
    };

    uint8_t response[128];
    uint8_t respLen = 0;

    Serial.printf("Setting max charge current: %.2f A -> raw value: %d (0x%04X)\n",
                  current, scaledValue, scaledValue);

    return sendCommand(0x01, 0x0E, cmdData, 2, response, &respLen);
}

bool MPPTController::LoadModeON(bool loadOn) {
    uint8_t response[128];
    uint8_t respLen = 0;

    if (loadOn){
        // return sendCommand(0x01, 0x2E, nullptr, 0, response, &respLen);
        return sendCommand(0x02, 0x00, nullptr, 0, response, &respLen);
    }else{
        //return sendCommand(0x01, 0x2F, nullptr, 0, response, &respLen);
        return sendCommand(0x02, 0x09, nullptr, 0, response, &respLen);
    }
}


bool MPPTController::setChargeVoltages(float buckVoltage, float floatVoltage) { // Unit is Volts
    uint8_t cmdData[4];
    uint16_t buckRaw = (uint16_t)(buckVoltage * 10);
    uint16_t floatRaw = (uint16_t)(floatVoltage * 10);

    cmdData[0] = (buckRaw >> 8) & 0xFF;
    cmdData[1] = buckRaw & 0xFF;
    cmdData[2] = (floatRaw >> 8) & 0xFF;
    cmdData[3] = floatRaw & 0xFF;

    // Send buck voltage (0x0F) and float voltage (0x10)
    uint8_t response[128];
    uint8_t respLen = 0;

    if (!sendCommand(0x01, 0x0F, cmdData, 2, response, &respLen)) return false;
    return sendCommand(0x01, 0x10, &cmdData[2], 2, response, &respLen);
}

bool MPPTController::setMachineAddress(uint8_t addr) {
    uint8_t cmdData[1] = {addr};
    uint8_t response[128];
    uint8_t respLen = 0;

    return sendCommand(0x01, 0x0B, cmdData, 1, response, &respLen);
}

bool MPPTController::clearTotalPower() {
    uint8_t response[128];
    uint8_t respLen = 0;

    return sendCommand(0x01, 0x0D, nullptr, 0, response, &respLen);
}

bool MPPTController::emergencyStop() {
    uint8_t cmdData[1] = {0x09};
    uint8_t response[128];
    uint8_t respLen = 0;

    return sendCommand(0x02, 0x09, nullptr, 0, response, &respLen);
}

bool MPPTController::clearEmergencyStop() {
    uint8_t cmdData[1] = {0x0A};
    uint8_t response[128];
    uint8_t respLen = 0;

    return sendCommand(0x02, 0x0A, nullptr, 0, response, &respLen);
}

bool MPPTController::resetToDefaults() {
    uint8_t response[128];
    uint8_t respLen = 0;

    return sendCommand(0x02, 0x00, nullptr, 0, response, &respLen);
}

// Getters
MPPTMeasurements* MPPTController::getMeasurements() {
    return &_measurements;
}

MPPTCal* MPPTController::getCalibration() {
    return &_calibration;
}

MPPTInfo* MPPTController::getDeviceInfo() {
    return &_info;
}

String MPPTController::getErrorString(uint16_t errorFlags) {
    String errors = "";

    if (errorFlags & (1 << ERROR_IN_RELAY)) errors += "Input relay failure, ";
    if (errorFlags & (1 << ERROR_IN_UVP)) errors += "Input voltage low, ";
    if (errorFlags & (1 << ERROR_IN_OVP)) errors += "Input voltage high, ";
    if (errorFlags & (1 << ERROR_BUCK1_OCP)) errors += "Buck1 overcurrent, ";
    if (errorFlags & (1 << ERROR_BUCK2_OCP)) errors += "Buck2 overcurrent, ";
    if (errorFlags & (1 << ERROR_OUT_UVP)) errors += "Output voltage low, ";
    if (errorFlags & (1 << ERROR_OUT_OVP)) errors += "Output voltage high, ";
    if (errorFlags & (1 << ERROR_OUT_RELAY)) errors += "Output relay failure, ";
    if (errorFlags & (1 << ERROR_OUT_SHORT)) errors += "Output short circuit, ";
    if (errorFlags & (1 << ERROR_POLAR)) errors += "Reverse polarity, ";
    if (errorFlags & (1 << ERROR_BAT_VOLT)) errors += "Battery voltage error, ";
    if (errorFlags & (1 << ERROR_BUCK1_FAULT)) errors += "Buck1 circuit fault, ";
    if (errorFlags & (1 << ERROR_D1_MOSFET)) errors += "D1 MOSFET failure, ";
    if (errorFlags & (1 << ERROR_BUCK2_FAULT)) errors += "Buck2 circuit fault, ";
    if (errorFlags & (1 << ERROR_D2_MOSFET)) errors += "D2 MOSFET failure, ";
    if (errorFlags & (1 << ERROR_BAT_OVER_TEMP)) errors += "Battery overtemperature, ";
    if (errorFlags & (1 << ERROR_ENVIR_OVER_TEMP)) errors += "Environment overtemperature, ";
    if (errorFlags & (1 << ERROR_BUCK_OVER_TEMP)) errors += "Buck overtemperature, ";
    if (errorFlags & (1 << ERROR_FAN1)) errors += "Fan1 failure, ";
    if (errorFlags & (1 << ERROR_FAN2)) errors += "Fan2 failure, ";
    if (errorFlags & (1 << ERROR_PV_VOLT_SENSOR)) errors += "PV voltage sensor fault, ";
    if (errorFlags & (1 << ERROR_BUS_VOLT_SENSOR)) errors += "Bus voltage sensor fault, ";
    if (errorFlags & (1 << ERROR_BUCK_VOLT_SENSOR)) errors += "Buck voltage sensor fault, ";
    if (errorFlags & (1 << ERROR_OUT_VOLT_SENSOR)) errors += "Output voltage sensor fault, ";
    if (errorFlags & (1 << ERROR_PV_CUR_SENSOR)) errors += "PV current sensor fault, ";
    if (errorFlags & (1 << ERROR_OUT_CUR_SENSOR)) errors += "Output current sensor fault, ";
    if (errorFlags & (1 << ERROR_BAT_OFF)) errors += "Battery turn off fault, ";
    if (errorFlags & (1 << ERROR_EMERGENCY_STOP)) errors += "Emergency stop active, ";
    if (errorFlags & (1 << ERROR_BUCK_OVP)) errors += "Buck overvoltage, ";

    if (errors.length() > 2) {
        errors.remove(errors.length() - 2);
    } else {
        errors = "No errors";
    }

    return errors;
}

String MPPTController::getModeString(MPPTMode mode) {
    switch(mode) {
        case MODE_POWER_ON: return "POWER_ON";
        case MODE_START_CHECK: return "START_CHECK";
        case MODE_NORMAL_RUN: return "NORMAL_RUN";
        case MODE_FAULT: return "FAULT";
        case MODE_UPDATE: return "UPDATE";
        case MODE_SETPAR: return "SETPAR";
        default: return "UNKNOWN";
    }
}

String MPPTController::getChargeModeString(ChargingMode mode) {
    switch(mode) {
        case CHARGE_NO_CHARGE: return "No Charge";
        case CHARGE_CC: return "Constant Current";
        case CHARGE_CV: return "Constant Voltage";
        case CHARGE_FLOAT: return "Float Charge";
        case CHARGE_SOFT_START: return "Soft Start";
        default: return "Unknown";
    }
}

String MPPTController::getBatteryTypeString(BatteryType type) {
    switch(type){
        case BATTERY_GEL1: return "Gel1";
        case BATTERY_AGM1: return "AGM1";
        case BATTERY_SEALED1: return "Sealed1";
        case BATTERY_FLOODED1: return "Floaded 1";
        case BATTERY_LITHIUM: return "Lithium";
        case BATTERY_GEL: return "Gel";
        case BATTERY_AGM: return "AGM";
        case BATTERY_SEALED: return "Sealed";
        case BATTERY_FLOODED: return "Floaded";
        case BATTERY_USER_DEFINE: return "UserDefinded";
        default: return "Unknown Battery Type";
    }
}

String MPPTController::getMachineTypeString(MachineType type) {
    switch(type) {
        case CHARGE: return "Solar Charger";
        case INVERTER: return "Inverter";
        case COMBO: return "Inverter Charger";
        default: return "Unknown";
    }
}

void MPPTController::clearBuffer() {
    while (_serial->available()) {
        _serial->read();
    }
}
