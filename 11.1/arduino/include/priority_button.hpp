#pragma once
#include <Arduino.h>

class IPriorityButton {
    public:
    virtual void check_triggered(bool *triggered);
};

class PriorityButton : public IPriorityButton {
    public:
    static volatile bool pressed;
    const int pin;

    static void button_isr();
    
    PriorityButton(int pin);
    void check_triggered(bool *triggered) override;
};