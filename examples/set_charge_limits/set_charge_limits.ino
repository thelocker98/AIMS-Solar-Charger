#include "MPPTController.h"

HardwareSerial mpptSerial(2);
MPPTController mppt(mpptSerial, 16, 17);

const uint16_t NEW_MAX_CHARGE_CURRENT = 60;  // A
const float NEW_BULK_CHARGE_VOLTAGE = 27.5f; // V
const float NEW_FLOAT_CHARGE_VOLTAGE = 27.0f; // V

void printCalibrationSummary() {
  MPPTCal* cal = mppt.getCalibration();

  Serial.println("Calibration Summary");
  Serial.println("-------------------");
  Serial.printf("Max Charge Current: %u A\n", cal->maxChargeCurrent);
  Serial.printf("Bulk (Buck) Voltage: %.1f V\n", cal->buckChargeVoltage);
  Serial.printf("Float Voltage: %.1f V\n", cal->floatChargeVoltage);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("AIMS MPPT Example - Set Charge Limits");

  if (!mppt.begin(9600)) {
    Serial.println("Failed to initialize controller.");
    Serial.println("Check wiring: RX=GPIO16, TX=GPIO17");
    return;
  }

  delay(3000);

  Serial.println("Current values:");
  if (mppt.readCalibration()) {
    printCalibrationSummary();
  } else {
    Serial.println("Failed to read calibration before update.");
  }

  delay(3000);
  Serial.println("Writing new limits...");

  bool currentOk = mppt.setMaxChargeCurrent(NEW_MAX_CHARGE_CURRENT);
  bool voltageOk = mppt.setChargeVoltages(NEW_BULK_CHARGE_VOLTAGE, NEW_FLOAT_CHARGE_VOLTAGE);

  Serial.printf("Set max charge current: %s\n", currentOk ? "OK" : "FAILED");
  Serial.printf("Set bulk/float voltages: %s\n", voltageOk ? "OK" : "FAILED");

  Serial.println();
  Serial.println("Values after write:");
  if (mppt.readCalibration()) {
    printCalibrationSummary();
  } else {
    Serial.println("Failed to read calibration after update.");
  }
}

void loop() {
  delay(2000);
}
