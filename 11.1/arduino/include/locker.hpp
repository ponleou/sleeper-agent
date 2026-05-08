#pragma once
#include <Servo.h>

class Locker {
    Servo servo;

    public:
        Locker(int pin);
        void lock();
        void unlock();
};
