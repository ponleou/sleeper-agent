#pragma once
#include <Servo.h>

class ILocker {
    public:
        virtual void lock();
        virtual void unlock();
};

// not enough power to move two servos at a time, do them one by one
// FS90MG does 0.12sec/60deg at 4.8V, so roughly 200ms for 90deg
#define SERVO_MOVE_COOLDOWN_MS 200

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
