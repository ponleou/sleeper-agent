#include "include/ble_host.hpp"

BleHost::BleHost()
    : service(BLE_SERVICE_UUID), session_id(BLE_SESSION_CHAR_UUID, BLERead | BLENotify, 6),
      data_enqueue(BLE_ENQUEUE_CHAR_UUID, BLERead | BLEWrite, 512),
      lock_action(BLE_LOCK_CHAR_UUID, BLEWrite), priority_data(BLE_PRIORITY_CHAR_UUID, BLEWrite, 512),
      alert_action(BLE_ALERT_CHAR_UUID, BLEWrite), weblink(BLE_WEBLINK_CHAR_UUID, BLEWrite, 512), metadata(BLE_METADATA_CHAR_UUID, BLERead, 512){

        this->lock_action.writeValue(false);
        this->priority_data.writeValue(" ");
        this->alert_action.writeValue(false);
        this->weblink.writeValue(" ");
}

void BleHost::initialise() {
    BLE.begin();
    BLE.setLocalName("SleeperAgent");

    this->service.addCharacteristic(this->data_enqueue);
    this->service.addCharacteristic(this->session_id);
    this->service.addCharacteristic(this->lock_action);
    this->service.addCharacteristic(this->priority_data);
    this->service.addCharacteristic(this->alert_action);
    this->service.addCharacteristic(this->weblink);
    this->service.addCharacteristic(this->metadata);

    BLE.addService(this->service);
    BLE.setAdvertisedService(this->service);
    BLE.advertise();
}

void BleHost::write_session_id(String id) {
    if (id.length() == 0)
        id = " ";

    this->session_id.writeValue(id);
}

bool BleHost::read_action_helper(BLEBoolCharacteristic action, bool *value) {
    bool read_value = action.value();
    *value = read_value;
    return read_value;
}

bool BleHost::read_action_char(BleHost::Action action, bool *value) {
    switch (action) {
    case BleHost::LOCK:
        return read_action_helper(this->lock_action, value);
        break;
    case BleHost::ALERT:
        return read_action_helper(this->alert_action, value);
        break;
    default:
        return false;
    }
}

bool BleHost::write_data_enqueue(queue<String> &queue) {
    if (this->data_enqueue.value() == " " && !queue.empty()) {
        String val = queue.front();

        // if data too long, we keep segment 1 2 and 4, and truncate segment 3 to fit
        if (val.length() > 512) {
            int d1 = val.indexOf('|');
            int d2 = val.indexOf('|', d1 + 1);
            int d3 = val.indexOf('|', d2 + 1);

            String seg1 = val.substring(0, d1);
            String seg2 = val.substring(d1 + 1, d2);
            String seg3 = val.substring(d2 + 1, d3);
            String seg4 = val.substring(d3 + 1);

            int fixedLen = seg1.length() + seg2.length() + 3 + seg4.length();
            int remaining = 512 - fixedLen;

            // if even truncating segment 3 doesnt fit, dont need to enqueue
            if (remaining < 0) {
                queue.pop();
                return false;
            }

            seg3 = seg3.substring(0, remaining);
            val = seg1 + "|" + seg2 + "|" + seg3 + "|" + seg4;
        }
        this->data_enqueue.writeValue(val);
        queue.pop();
        return true;
    }
    return false;
}

void BleHost::reset_enqueued() {
    if (this->data_enqueue.value() != " ")
        this->data_enqueue.writeValue(" ");
}

bool BleHost::read_weblink_char(String *value) {
    if (this->weblink.value() != " ") {
        *value = weblink.value();
        return true;
    }

    return false;
}

bool BleHost::read_priority_char(String *value) {
    if (this->priority_data.value() != " ") {
        *value = priority_data.value();
        return true;
    }
    return false;
}

void BleHost::write_metadata(String metadata) {
    if (metadata == "")
        metadata = " ";
    this->metadata.writeValue(metadata);
}

void BleHost::reset_weblink() {
    if (this->weblink.value() != " ")
        this->weblink.writeValue(" ");
}

void BleHost::reset_actions() {
    this->lock_action.writeValue(false);
    this->alert_action.writeValue(false);
}

void BleHost::reset_ble() {
    this->write_session_id(" ");
    this->write_metadata(" ");
    this->reset_enqueued();
    this->reset_weblink();
    this->reset_actions();
}

void BleHost::debug_print() {
    // Serial.println(this->data_enqueue.value());
    // Serial.println(this->data_enqueue.valueLength());
    // Serial.println(this->data_enqueue.value() == " ");
}