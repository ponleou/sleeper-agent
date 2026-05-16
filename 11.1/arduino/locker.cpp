#include <Arduino.h>
#include "include/locker.hpp"

Locker::Locker(int left_pin, int right_pin) {
    this->left_servo.attach(left_pin);
    this->right_servo.attach(right_pin);
    
    this->left_servo.write(0);
    delay(SERVO_MOVE_COOLDOWN_MS);
    this->right_servo.write(90);
    delay(SERVO_MOVE_COOLDOWN_MS);

    this->left_locked = false;
    this->right_locked = false;

    this->last_moved_ms = 0;
}

void Locker::lock() {
    if (millis() - this->last_moved_ms < SERVO_MOVE_COOLDOWN_MS) 
        return;

    if (!left_locked) {
        this->last_moved_ms = millis();
        this->left_servo.write(90);
        this->left_locked = true;
        return;
    }

    if (!right_locked) {
        this->last_moved_ms = millis();
        this->right_servo.write(0);
        this->right_locked = true;
        return;
    }
}


void Locker::unlock() {
    if (millis() - this->last_moved_ms < SERVO_MOVE_COOLDOWN_MS) 
        return;

    if (left_locked) {
        this->last_moved_ms = millis();
        this->left_servo.write(0);
        this->left_locked = false;
        return;
    }

    if (right_locked) {
        this->last_moved_ms = millis();
        this->right_servo.write(90);
        this->right_locked = false;
        return;
    }
}