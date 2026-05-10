#pragma once
#include "alarm.hpp"
#include "ble_host.hpp"
#include "locker.hpp"
#include "nfc_reader.hpp"
#include "priority_button.hpp"
#include "screen.hpp"

#define PRIORITY_ACTION_DISMISS_DURATION_MS 6000
#define PRIORITY_TIME_DURATION_MS 12000
#define UNOCCURING_PRIORITY_COUNT_THRESHOLD 10

class Actuators {
  private:
    IBleHostActuatorReader &host_reader;
    IAlarm &alarm;
    ILocker &locker;
    INfcActuator &nfc;
    IScreen &screen;
    IPriorityButton &priority_button;

    bool priority_exec_once;
    bool priority_action;
    unsigned long priority_action_start_time_ms;
    bool priority_running;
    unsigned long priority_running_start_time_ms;
    uint8_t unoccuring_priority_count;

    void reset_priority_states();
    void reset();

  public:
    Actuators(IBleHostActuatorReader &host_reader, IAlarm &alarm, ILocker &locker, INfcActuator &nfc, IScreen &screen, IPriorityButton &button);

    void update();
};