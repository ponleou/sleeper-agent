#ifdef __CLANGD__
#include <Arduino.h>
#endif

#define PN532_HSU_IMPLEMENTATION 1
#include "include/nfc_reader.hpp"

NfcReader nfc(Serial1);

void setup(void) {
    Serial.begin(115200);
    while (!Serial) {
    }

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