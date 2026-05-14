#include "MPPTController.h"

// Create serial instance for UART2 (pins 16=RX, 17=TX)
HardwareSerial mpptSerial(2);
MPPTController mppt(mpptSerial, 0x01, 16, 17);

void setup() {
    Serial.begin(115200);
    Serial.println("\nMPPT Solar Controller Library");
    Serial.println("Example: Basic Reading\n");

    // Initialize the MPPT controller
    if (mppt.begin(9600)) {
        Serial.println("MPPT Controller initialized successfully.\n");
    } else {
        Serial.println("Failed to initialize MPPT Controller");
        Serial.println("   Check wiring: RX=GPIO16, TX=GPIO17");
    }
}

void loop() {
    if (!mppt.readMeasurements()) {
        Serial.println("Read failed.");
        delay(3000);
        return;
    }

    MPPTMeasurements* data = mppt.getMeasurements();
    MPPTInfo* info = mppt.getDeviceInfo();


    Serial.println("--------------------------------------------");
    Serial.println("Current Solar Status");
    Serial.println("--------------------------------------------");

    Serial.printf("PV Voltage:        %.1f V\n", data->pvVoltage);
    Serial.printf("Bus Voltage:       %.1f V\n", data->busVoltage);
    Serial.printf("Battery Voltage:   %.1f V\n", data->batteryVoltage);
    Serial.printf("Charging Current:  %.2f A\n", data->chargingCurrent);
    Serial.printf("Charging Power:    %.1f W\n", data->chargingPower);
    Serial.printf("Battery Temp:      %.1f C\n", data->batteryTemp);
    Serial.printf("Environment Temp:  %.1f C\n", data->environmentTemp);

    Serial.printf("\nOperating Mode:    %s\n", mppt.getModeString(data->operatingMode).c_str());
    Serial.printf("Charging Mode:     %s\n", mppt.getChargeModeString(data->chargingMode).c_str());

    Serial.println();

    Serial.printf("Day Power:         %lu Wh\n", (unsigned long)data->dayPower);
    Serial.printf("Total Power:       %lu Wh\n", (unsigned long)data->totalPower);
    Serial.printf("Day Energy:        %.2f kWh\n", data->dayPower / 1000.0);
    Serial.printf("Total Energy:      %.2f kWh\n", data->totalPower / 1000.0);
    Serial.printf("Error Flags:       0x%04X\n", data->errorFlags);

    if (data->errorFlags != 0) {
      Serial.printf("\nErrors: %s\n", mppt.getErrorString(data->errorFlags).c_str());
    } else {
      Serial.println("\nSystem Status: NORMAL");
    }

    Serial.println("--------------------------------------------\n");

    delay(3000);
}
