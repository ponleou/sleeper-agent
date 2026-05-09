#pragma once
#include "alarm.hpp"
#include "ble_host.hpp"
#include "locker.hpp"
#include "nfc_reader.hpp"

class Actuators {
  private:
    IBleHostActuatorReader &host_reader;
    Alarm &alarm;
    Locker &locker;
    INfcActuator &nfc;

    void reset();

  public:
    Actuators(IBleHostActuatorReader &host_reader, Alarm &alarm, Locker &locker, INfcActuator &nfc);

    void update();
};