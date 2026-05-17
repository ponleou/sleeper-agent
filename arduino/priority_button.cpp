#include "include/priority_button.hpp"

volatile bool PriorityButton::pressed = false;

PriorityButton::PriorityButton(int pin) : pin(pin) {
  pinMode(this->pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(this->pin), PriorityButton::button_isr, FALLING);
}

void PriorityButton::button_isr() {
    PriorityButton::pressed = true;
}

void PriorityButton::check_triggered(bool *triggered) {
  if (PriorityButton::pressed) {
    *triggered = true;
    PriorityButton::pressed = false;
  }
}