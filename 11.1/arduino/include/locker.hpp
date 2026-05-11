#pragma once
#include <Servo.h>

class ILocker {
    public:
        virtual void lock();
        virtual void unlock();
};

// not enough power to move two servos at a time, do them one by one
#define SERVO_MOVE_COOLDOWN_MS 1000

class Locker : public ILocker {
    Servo left_servo;
    Servo right_servo;

    bool left_locked;
    bool right_locked;

    unsigned long last_moved_ms;

    public:
        Locker(int left_pin, int right_pin);
        void lock() override;
        void unlock() override;
};
