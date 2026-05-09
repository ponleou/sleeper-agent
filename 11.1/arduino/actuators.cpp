#include "include/actuators.hpp"

Actuators::Actuators(IBleHostActuatorReader &host_reader, Alarm &alarm, Locker &locker, INfcActuator &nfc)
    : host_reader(host_reader), alarm(alarm), locker(locker), nfc(nfc) {
}

void Actuators::reset() {
    this->alarm.off();
    this->locker.unlock();
    this->nfc.stop_action();
}

void Actuators::update() {
    if (!this->host_reader.read_server_status()) {
        this->reset();
        return;
    }

    bool alert = false;
    this->host_reader.read_action_char(IBleHostActuatorReader::ALERT, &alert);
    if (alert) {
        Serial.println("ALERT ACTION");
        this->alarm.toggle();
    } else {
        this->alarm.off();
    }

    bool start = false;
    this->host_reader.read_action_char(IBleHostActuatorReader::LOCK, &start);
    if (start) {
        Serial.println("START ACTION");
        this->locker.lock();
        this->nfc.start_action();
    } else {
        Serial.println("STOP ACTION");
        this->locker.unlock();
        this->nfc.stop_action();
    }
}