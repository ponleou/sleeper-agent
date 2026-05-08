#pragma once
#include "BLEStringCharacteristic.h"
#include <ArduinoBLE.h>
#include <queue>
using std::queue;

#define BLE_SERVICE_UUID "00000100-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_SESSION_CHAR_UUID "00000101-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_ENQUEUE_CHAR_UUID "00000110-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_START_CHAR_UUID "00000120-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_PRIORITY_CHAR_UUID "00000130-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_ALERT_CHAR_UUID "00000140-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_WEBLINK_CHAR_UUID "00000150-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_METADATA_CHAR_UUID "00000160-addd-43a2-b9cc-6c8adc8a7761"

class IBleHostWriter {
  public:
    enum Action {
        START,
        ALERT,
    };

    virtual void write_session_id(String id);
    virtual bool write_data_enqueue(queue<String> &queue);
    virtual void write_metadata(String metadata);
    virtual void reset_ble();
    
    // FIXME: move this to a Reader
    // this will also require moving implementations of nfc_reader to an actuator
    virtual bool read_weblink_char(String *value);
    virtual bool read_action_char(IBleHostWriter::Action action, bool *value);
    virtual bool read_priority_char(String *value);

    virtual void debug_print();
};

class BleHost : public IBleHostWriter {
  private:
    BLEService service;
    BLEStringCharacteristic data_enqueue;
    BLEStringCharacteristic session_id;
    BLEBoolCharacteristic start_action;
    BLEStringCharacteristic priority_data;
    BLEBoolCharacteristic alert_action;
    BLEStringCharacteristic weblink;
    BLEStringCharacteristic metadata;

    bool read_action_helper(BLEBoolCharacteristic action, bool *value);

  public:
    BleHost();
    void initialise();
    void reset_enqueued();
    void reset_weblink();
    void reset_actions();

    void write_session_id(String id) override;
    bool write_data_enqueue(queue<String> &queue) override;
    void write_metadata(String metadata) override;
    void reset_ble() override;

    bool read_weblink_char(String *value) override;
    bool read_action_char(BleHost::Action action, bool *value) override;
    bool read_priority_char(String *value) override;

    void debug_print() override;
};