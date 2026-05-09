#pragma once
#include <Arduino.h>

#define ALARM_FREQ 3000

class Alarm {
  private:
    const int buzzer;
    bool is_on;

  public:
    Alarm(int pin);

    void off();
    void on();
    void toggle();
};