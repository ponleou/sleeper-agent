#ifdef __CLANGD__
#include <Arduino.h>
#endif

#define PN532_HSU_IMPLEMENTATION 1
#include "include/ble_host.hpp"
#include "include/nfc_reader.hpp"
#include "include/alarm.hpp"
#include "include/locker.hpp"
#include "include/actuators.hpp"
#include "include/priority_button.hpp"

#define SCREEN_SENSOR_PIN 2
#define PRIORITY_BUTTON_PIN 3

#define ALARM_PIN 5
// #define ALARM_PIN 7

#define LOCK_RIGHT_PIN 12
#define LOCK_LEFT_PIN 10

#define SLOW_POLLING_PERIOD 500

BleHost host;
NfcReader nfc(Serial1, host);

Alarm alarm(ALARM_PIN);
Locker locker(LOCK_LEFT_PIN, LOCK_RIGHT_PIN);
Screen screen(SCREEN_SENSOR_PIN, "Hello", "Setting up...");
PriorityButton button(PRIORITY_BUTTON_PIN);

Actuators actuate(host, alarm, locker, nfc, screen, button);

unsigned long slow_polling_last_ms = millis();

void setup(void) {
    screen.initialise();

    Serial.begin(115200);

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
        actuate.update();
    }
}
