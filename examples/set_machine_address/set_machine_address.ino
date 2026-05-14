#include "MPPTController.h"

HardwareSerial mpptSerial(2);
MPPTController mppt(mpptSerial, 0x01, 16, 17);


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Changing address");

  if (!mppt.begin(9600)) {
    Serial.println("Failed to initialize controller.");
    return;
  }
}

void loop() {
  // Change Address
  mppt.setAddress(0x01); // Change Driver address
  bool currentOk = mppt.setMachineAddress(0x02); // Change Device address

  Serial.println(currentOk ? "success" : "error");
  delay(2000);

  mppt.setAddress(0x02); // Change Driver address

  if (!mppt.readMeasurements()) {
    Serial.println("Read failed.");
    delay(3000);
    return;
  }
  BATT

  MPPTMeasurements* data = mppt.getMeasurements();
  MPPTInfo* info = mppt.getDeviceInfo();

  Serial.printf("Bus Voltage:       %.1f V\n", data->busVoltage);
  Serial.printf("Battery Voltage:   %.1f V\n", data->batteryVoltage);
  Serial.printf("Charging Current:  %.2f A\n", data->chargingCurrent);

  delay(2000);
}
