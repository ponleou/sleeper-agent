#pragma once
#include <ArduinoBLE.h>
#include <queue>
using std::queue;

#define BLE_SERVICE_UUID "00000100-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_SESSION_CHAR_UUID "00000101-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_ENQUEUE_CHAR_UUID "00000110-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_START_CHAR_UUID "00000120-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_STOP_CHAR_UUID "00000130-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_ALERT_CHAR_UUID "00000140-addd-43a2-b9cc-6c8adc8a7761"
#define BLE_WEBLINK_CHAR_UUID "00000150-addd-43a2-b9cc-6c8adc8a7761"

class IBleHostWriter {
  public:
    virtual void set_session_id(String id);
    virtual bool enqueue_data(queue<String> &queue);
    virtual void reset_enqueued();

    virtual void debug_print();
};

class BleHost : public IBleHostWriter {
  private:
    BLEService service;
    BLEStringCharacteristic data_enqueue;
    BLEStringCharacteristic session_id;
    BLEBoolCharacteristic start_action;
    BLEBoolCharacteristic stop_action;
    BLEBoolCharacteristic alert_action;
    BLEStringCharacteristic weblink;

    bool read_action_helper(BLEBoolCharacteristic action, bool *value);

  public:

    enum Action {
        START,
        STOP,
        ALERT,
    };

    BleHost();
    void initialise();
    bool read_action_char(BleHost::Action action, bool *value);
    void set_session_id(String id) override;
    bool enqueue_data(queue<String> &queue) override;
    void reset_enqueued() override;
    bool read_weblink_char(String *value);

    void debug_print() override;
};