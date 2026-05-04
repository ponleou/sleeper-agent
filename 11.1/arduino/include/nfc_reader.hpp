#pragma once
#define NFC_INTERFACE_HSU

#include <PN532.h>
#include <PN532_HSU.h>

#ifndef PN532_HSU_IMPLEMENTATION
#define PN532_HSU_IMPLEMENTATION 0
#endif
#if PN532_HSU_IMPLEMENTATION
#include <PN532_HSU.cpp>
#endif

#define CLA_STANDARD 0x00
#define CLA_PROPRIETARY 0x80

#define INS_IDENTIFY 0x01
#define LE_IDENTITY 0x06 // 6 characters for ID

#define P_NULL 0x00

#define INS_SELECT 0xA4
#define INS_SELECT_P1_AID 0x04
#define INS_SELECT_P1_FIRST 0x00

class NfcReader {
  private:
    PN532_HSU pn532hsu;
    PN532 nfc;
    bool connected;
    bool selected;

    void check_connection();
    void select_hce();
    void communicate();

  public:
    uint32_t version_data;

    NfcReader(HardwareSerial &serial);

    bool initialise();
    void stateful_communication();
};

class NfcCommands {
    protected:
        static void identify(PN532 nfc);

    friend NfcReader;
};