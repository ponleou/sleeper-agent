#ifdef __CLANGD__
#include <Arduino.h>
#endif

#define PN532_HSU_IMPLEMENTATION 1
#include "include/ble_host.hpp"
#include "include/nfc_reader.hpp"
#include "include/locker.hpp"

#define SLOW_POLLING_PERIOD 500

BleHost host;
NfcReader nfc(Serial1, host);
Locker locker(9);

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

        bool test = false;
        if (host.read_action_char(BleHost::LOCK, &test))
            locker.lock();
        else
            locker.unlock();
    }
}