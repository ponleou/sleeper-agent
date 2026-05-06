#ifdef __CLANGD__
#include <Arduino.h>
#endif

#define PN532_HSU_IMPLEMENTATION 1
#include "include/nfc_reader.hpp"
#include "include/ble_host.hpp"

BleHost host;
NfcReader nfc(Serial1, host);

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
    nfc.stateful_communication();
}