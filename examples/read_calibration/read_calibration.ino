#include "MPPTController.h"

HardwareSerial mpptSerial(2);
MPPTController mppt(mpptSerial, 16, 17);

void printCalibration() {
  MPPTCal* cal = mppt.getCalibration();

  Serial.println("Calibration Data");
  Serial.println("----------------");
  Serial.printf("Address: 0x%02X\n", cal->machineAddress);
  Serial.printf("Battery Type: %s\n", mppt.getBatteryTypeString(cal->batteryType).c_str());
  Serial.printf("Max Charge Current: %u A\n", cal->maxChargeCurrent);
  Serial.printf("Buck Charge Voltage: %.1f V\n", cal->buckChargeVoltage);
  Serial.printf("Float Charge Voltage: %.1f V\n", cal->floatChargeVoltage);
  Serial.println();
  Serial.printf("PV Voltage Offset: %d\n", cal->pvVoltOffset);
  Serial.printf("PV Voltage Ratio: %u\n", cal->pvVoltRatio);
  Serial.printf("Output Voltage Offset: %d\n", cal->outVoltOffset);
  Serial.printf("Output Voltage Ratio: %u\n", cal->outVoltRatio);
  Serial.printf("Output Current Offset: %d\n", cal->outCurOffset);
  Serial.printf("Output Current Ratio: %lu\n", (unsigned long)cal->outCurRatio);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("AIMS MPPT Example - Read Calibration");

  if (!mppt.begin(9600)) {
    Serial.println("Failed to initialize controller.");
    Serial.println("Check wiring: RX=GPIO16, TX=GPIO17");
    return;
  }
}

void loop() {
  if (mppt.readCalibration()) {
    printCalibration();
  } else {
    Serial.println("Failed to read calibration data.");
  }
  delay(2000);
}
