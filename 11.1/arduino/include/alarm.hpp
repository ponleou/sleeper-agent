#pragma once
#include <Arduino.h>

#define ALARM_FREQ 3000

class IAlarm {
  public:
    virtual void off();
    virtual void on();
    virtual void toggle();
};

class Alarm : public IAlarm {
  private:
    const int buzzer;
    bool is_on;

  public:
    Alarm(int pin);

    void off() override;
    void on() override;
    void toggle() override;
};