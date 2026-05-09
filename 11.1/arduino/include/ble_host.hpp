#pragma once
#include "BLEStringCharacteristic.h"
#include <ArduinoBLE.h>
#include <queue>
using std::queue;

#define BLE_SERVICE_UUID "00000100-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_SESSION_CHAR_UUID "00000101-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_ENQUEUE_CHAR_UUID "00000110-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_LOCK_CHAR_UUID "00000120-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_PRIORITY_CHAR_UUID "00000130-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_ALERT_CHAR_UUID "00000140-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_WEBLINK_CHAR_UUID "00000150-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_METADATA_CHAR_UUID "00000160-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_SERVERPOLL_CHAR_UUID "00000170-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_STATUS_CHAR_UUID "00000180-addd-43a2-b9cc-6c8adc8a7761"

#define SERVER_TIMEOUT_TOLERANCE_MS 10000

class IBleHostActuatorReader {
  public:
    enum Action {
        LOCK,
        ALERT,
    };

    virtual bool read_server_status();
    virtual bool read_action_char(IBleHostActuatorReader::Action action, bool *value);
    virtual bool read_status_text_char(String *value);
};

// required by NfcReader to communicate
class IBleHostStateCommunicator {
  public:
    virtual bool poll_server_status();
    virtual void write_session_id(String id);
    virtual void write_metadata(String metadata);
    virtual bool write_data_enqueue(queue<String> &queue);
    virtual bool read_priority_char(String *value);
    virtual bool read_weblink_char(String *value);
    virtual void reset_ble();
    
    // FIXME: move this to a Reader
    // this will also require moving implementations of nfc_reader to an actuator

    virtual void debug_print();
};

class BleHost : public IBleHostStateCommunicator, public IBleHostActuatorReader {
  private:
    BLEService service;
    
    // reads characteristics
    BLEStringCharacteristic data_enqueue;
    BLEStringCharacteristic session_id;
    BLEStringCharacteristic metadata;

    // writes characteristics
    BLEBoolCharacteristic lock_action;
    BLEStringCharacteristic priority_data;
    BLEBoolCharacteristic alert_action;
    BLEStringCharacteristic weblink;
    BLEStringCharacteristic status_text;
    BLEIntCharacteristic server_poll;

    // for server poll
    unsigned long last_server_poll_ms;

    bool read_action_helper(BLEBoolCharacteristic &action, bool *value);

  public:
    BleHost();
    void initialise();
    void reset_enqueued();
    void reset_weblink();

    bool poll_server_status() override;
    void write_session_id(String id) override;
    void write_metadata(String metadata) override;
    bool write_data_enqueue(queue<String> &queue) override;
    bool read_weblink_char(String *value) override;
    bool read_priority_char(String *value) override;
    void reset_ble() override;

    bool read_server_status() override;
    bool read_action_char(BleHost::Action action, bool *value) override;
    bool read_status_text_char(String *value) override;
    // void reset_actions() override;

    void debug_print() override;
};