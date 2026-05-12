#include "include/actuators.hpp"

Actuators::Actuators(IBleHostActuatorReader &host_reader, IAlarm &alarm, ILocker &locker, INfcActuator &nfc,
                     IScreen &screen, IPriorityButton &button)
    : host_reader(host_reader), alarm(alarm), locker(locker), nfc(nfc), screen(screen), priority_button(button) {
    this->reset_priority_states();
}

void Actuators::reset_priority_states() {
    this->priority_action = false;
    this->priority_action_start_time_ms = 0;
    this->priority_running_start_time_ms = 0;
    this->priority_running = false;
    this->unoccuring_priority_count = 0;
    this->priority_exec_once = false;
}

void Actuators::reset() {
    this->alarm.off();
    this->locker.unlock();
    this->nfc.stop_action();
    this->reset_priority_states();
}

void Actuators::update() {

    if (!this->host_reader.read_server_status()) {
        this->screen.set_text(":(");
        this->screen.set_subtext("Server disconnected");
        this->reset();
        this->screen.update();
        return;
    }

    bool alert_action = false;
    this->host_reader.read_action_char(IBleHostActuatorReader::ALERT, &alert_action);
    bool start_action = false;
    this->host_reader.read_action_char(IBleHostActuatorReader::START, &start_action);

    this->priority_button.check_triggered(&this->priority_action);

    // if priority is happening, then we unlock
    if (this->priority_action) {
        this->screen.set_text("!!!");

        if (!this->priority_exec_once) {
            this->priority_exec_once = true;
            this->priority_action_start_time_ms = millis();
        }

        this->alarm.off();
        this->locker.unlock();

        // if it is alert action, while priority is still going, it means the phone is out of the lock, and priority
        // action officially starts we give a timer for priority action to last after the timer, priority will go off
        if (alert_action) {
            if (!this->priority_running) {
                this->priority_running = true;
                this->priority_running_start_time_ms = millis();

            } else if (millis() - this->priority_running_start_time_ms >= PRIORITY_TIME_DURATION_MS) {
                this->reset_priority_states();

            }

            int time_remaining =
                max(0, (int)(PRIORITY_TIME_DURATION_MS - (millis() - this->priority_running_start_time_ms)) / 1000);

            this->screen.set_subtext("Break time for " + String(time_remaining) + "s");

        }

        // if it is start action, it means the phone was previously in lock
        // we alert for priority action (important notification)
        else if (start_action) {
            // if priority hasnt start (or becoome active yet), alert so it can be active
            if (!priority_running) {
                this->alarm.on();

                // if passed dismiss time, then stop priority action
                if (millis() - this->priority_action_start_time_ms >= PRIORITY_ACTION_DISMISS_DURATION_MS) {
                    this->reset_priority_states();
                }

                int time_remaining = max(
                    0, (int)(PRIORITY_ACTION_DISMISS_DURATION_MS - (millis() - this->priority_action_start_time_ms)) /
                           1000);
                this->screen.set_subtext("Dismissed in " + String(time_remaining) + "s");
            }
            // // if it has already been active, and the phone is back in the lock, assume they finished with their
            // // priority action end priority action and continue with process
            else {
                this->reset_priority_states();
            }
        }

        // if not alert or start, then it is passed start time, and there shouldnt be any more action
        // turn off priority because theres no need for anything
        // NOTE: there is a delay between changing from start to alert and such, so theres a timegap where both is
        // false, but will turn one of the states later
        else {
            if (this->unoccuring_priority_count < UNOCCURING_PRIORITY_COUNT_THRESHOLD) {
                this->unoccuring_priority_count++;
            } else {
                this->reset_priority_states();
            }
        }


        this->screen.update(true);

        return;
    }

    if (alert_action) {
        this->screen.set_text(":<");
        this->screen.set_subtext("It is bedtime!");
        this->alarm.toggle();
        this->screen.update(true);
    } else {
        this->alarm.off();
    }

    if (start_action) {
        this->screen.set_text("GOOD NIGHT");
        this->screen.set_subtext("Go to sleep.");
        this->locker.lock();
        this->nfc.start_action(&this->priority_action);
        this->screen.update();
    } else {
        this->locker.unlock();
        this->nfc.stop_action();
    }

    if (!start_action && !alert_action) {
        this->screen.set_text(":)");
        this->screen.set_subtext("Hello!");
        this->screen.update();
    }
}
