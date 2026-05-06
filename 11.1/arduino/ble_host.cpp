#include "include/ble_host.hpp"

BleHost::BleHost()
    : service(BLE_SERVICE_UUID), session_id(BLE_SESSION_CHAR_UUID, BLERead, 6),
      data_enqueue(BLE_DATAQUEUE_CHAR_UUID, BLERead | BLEWrite | BLENotify, 512),
      start_action(BLE_START_CHAR_UUID, BLEWrite), stop_action(BLE_STOP_CHAR_UUID, BLEWrite),
      alert_action(BLE_ALERT_CHAR_UUID, BLEWrite) {
}

void BleHost::initialise() {
    BLE.begin();
    BLE.setLocalName("SleeperAgent");

    this->service.addCharacteristic(this->data_enqueue);
    this->service.addCharacteristic(this->start_action);
    this->service.addCharacteristic(this->stop_action);
    this->service.addCharacteristic(this->alert_action);

    BLE.addService(this->service);
    BLE.setAdvertisedService(this->service);
    BLE.advertise();
}

void BleHost::set_session_id(String id) {
    this->session_id.writeValue(id);
}

bool BleHost::read_action_helper(BLEBoolCharacteristic action, bool *value) {
    if (action.written()) {
        *value = action.value();
        return true;
    }
    return false;
}

bool BleHost::read_action_char(BleHost::Action action, bool *value) {
    switch (action) {
    case BleHost::START:
        return read_action_helper(this->start_action, value);
        break;
    case BleHost::STOP:
        return read_action_helper(this->stop_action, value);
        break;
    case BleHost::ALERT:
        return read_action_helper(this->alert_action, value);
        break;
    default:
        return false;
    }
}

bool BleHost::enqueue_data(queue<String> &queue) {
    if (this->data_enqueue.written() && !queue.empty()) {
        String val = queue.front();
        if (val.length() > 512)
            val = val.substring(0, 512);
        this->data_enqueue.writeValue(val);
        queue.pop();
        return true;
    }
    return false;
}