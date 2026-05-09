#include <Arduino.h>
#include "include/locker.hpp"

Locker::Locker(int pin) {
    // this->servo.attach(pin);
    // this->servo.write(0);
}

void Locker::lock() {
    // Serial.println("lock");
    // this->servo.write(90);
}


void Locker::unlock() {
    // Serial.println("unlock");
    // this->servo.write(0);
}