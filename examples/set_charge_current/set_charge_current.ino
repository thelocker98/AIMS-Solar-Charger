#include "MPPTController.h"

HardwareSerial mpptSerial(2);
MPPTController mppt(mpptSerial, 0x01, 16, 17);


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("AIMS MPPT Example - Set Charge Limits");

  if (!mppt.begin(9600)) {
    Serial.println("Failed to initialize controller.");
    return;
  }
}

void loop() {
  Serial.println("Writing new limits...");
  bool currentOk = mppt.setMaxChargeCurrent(NEW_MAX_CHARGE_CURRENT);

  Serial.println(currentOk ? "success" : "error");
  delay(3000);
}
