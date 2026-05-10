#pragma once
#include <Servo.h>

class ILocker {
    public:
        virtual void lock();
        virtual void unlock();
};

class Locker : public ILocker {
    Servo servo;

    public:
        Locker(int pin);
        void lock() override;
        void unlock() override;
};
