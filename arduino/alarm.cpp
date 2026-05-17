#include "include/alarm.hpp"

Alarm::Alarm(int pin) : buzzer(pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(this->buzzer, LOW);
    this->is_on = false;
}

void Alarm::off() {
    noTone(this->buzzer);
    this->is_on = false;
}

void Alarm::on() {
    tone(this->buzzer, ALARM_FREQ);
    this->is_on = true;
}

void Alarm::toggle() {
    if (this->is_on)
        this->off();
    else
        this->on();
}
