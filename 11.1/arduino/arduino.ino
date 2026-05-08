#ifdef __CLANGD__
#include <Arduino.h>
#endif

#define PN532_HSU_IMPLEMENTATION 1
#include "include/nfc_reader.hpp"
#include "include/ble_host.hpp"

#define SLOW_POLLING_PERIOD 500

BleHost host;
NfcReader nfc(Serial1, host);

unsigned long slow_polling_last_ms = millis();


void setup(void) {
    Serial.begin(115200);
    while (!Serial) {
    }

    host.initialise();

    Serial1.begin(115200);
    if (!nfc.initialise()) {
        Serial.println("Failed to start NFC");
        while (true) {
        }
    }
}

void loop() {
    BLE.poll();

    if (millis() - slow_polling_last_ms >= SLOW_POLLING_PERIOD) {
        slow_polling_last_ms = millis();

        // run slow pollers here
        nfc.stateful_communication();
    }

}