#pragma once
#define NFC_INTERFACE_HSU

#include <PN532.h>
#include <PN532_HSU.h>
#include <queue>
#include "ble_host.hpp"
using std::queue;

#ifndef PN532_HSU_IMPLEMENTATION
#define PN532_HSU_IMPLEMENTATION 0
#endif
#if PN532_HSU_IMPLEMENTATION
#include <PN532_HSU.cpp>
#endif

#define POLLING_PERIOD 500

#define CLA_PROPRIETARY 0x80

#define INS_IDENTIFY 0x01
#define INS_POLL 0x02
#define INS_COLLECT 0x03
#define INS_START_CLIENT_COLLECTOR 0x04
#define INS_STOP_CLIENT_COLLECTOR 0x05

#define P_NULL 0x00
#define P1_COLLECT_PROVIDE_LENGTH 0x01
#define P2_COLLECT_LENGTH_POS 0x00

#define LE_ID_LENGTH 0x06 // 6 characters for ID
#define LE_ALL 0x00

#define CLA_STANDARD 0x00

#define INS_SELECT 0xA4
#define INS_SELECT_P1_AID 0x04
#define INS_SELECT_P2_FIRST 0x00

#define SW1_DATA 0x61
#define SW1_SUCCESS 0x90
#define SW2_SUCCESS 0x00

class NfcReader {
  private:
    PN532_HSU pn532hsu;
    PN532 nfc;
    IBleHostWriter &host;

    bool connected;
    bool selected;
    bool identified;
    bool client_collector_active;

    unsigned long last_communication_ms;
    String identity;
    queue<String> collected_queue;

    void check_connection();
    void select_hce();
    void communicate();
    void reset_state();

  public:
    uint32_t version_data;

    NfcReader(HardwareSerial &serial, IBleHostWriter &host);

    bool initialise();
    void stateful_communication();
};

class NfcCommands {
  protected:
    static bool identify(PN532 &nfc, String *value);
    static bool poll (PN532 &nfc, bool *initiate_collect);
    static bool collect(PN532 &nfc, bool *initiate_collect, uint8_t data[], uint8_t *data_length);
    static bool start_client_collector(PN532 &nfc);
    static bool stop_client_collector(PN532 &nfc);

    friend NfcReader;
};
