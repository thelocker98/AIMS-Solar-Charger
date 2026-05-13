#include "MPPTController.h"

HardwareSerial mpptSerial(2);
MPPTController mppt(mpptSerial, 16, 17);

void printDeviceInfo() {
  MPPTInfo* info = mppt.getDeviceInfo();

  Serial.println("Device Information");
  Serial.println("------------------");
  Serial.printf("Machine Type: %u\n",  mppt.getMachineTypeString(info->machineType));
  Serial.printf("Serial Number: %s\n", info->serialNumber;
  Serial.printf("Model: %s\n", info->machineModel);
  Serial.printf("Main Firmware: %s\n", info->mainFirmware);
  Serial.printf("Slave Firmware: %s\n", info->slaveFirmware);
  Serial.printf("Battery Type: %s\n", mppt.getBatteryTypeString(cal->batteryType));
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("AIMS MPPT Example - Read Device Info");

  if (!mppt.begin(9600)) {
    Serial.println("Failed to initialize controller.");
    Serial.println("Check wiring: RX=GPIO16, TX=GPIO17");
    return;
  }


}

void loop() {
  if (mppt.readDeviceInfo()) {
    printDeviceInfo();
  } else {
    Serial.println("Failed to read device information.");
  }
  delay(2000);
}
