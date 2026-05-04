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