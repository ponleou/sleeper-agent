#include "include/nfc_reader.hpp"

NfcReader::NfcReader(HardwareSerial &serial) : pn532hsu(serial), nfc(pn532hsu) {
    this->version_data = 0;
    this->connected = false;
    this->selected = false;
}

void NfcReader::check_connection() {
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;
    this->connected = this->nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, 20, true);

    if (this->connected)
        Serial.print("Connected");
}

void NfcReader::select_hce() {
    uint8_t selectApp[] = {0x00, 0xA4, 0x04, 0x00, 0x07, 0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00};
    uint8_t resLen = 255;
    uint8_t response[resLen];

    if (this->nfc.inDataExchange(selectApp, sizeof(selectApp), response, &resLen))
        this->selected = response[resLen - 2] == 0x90 && response[resLen - 1] == 0x00;
}

void NfcReader::communicate() {
}

bool NfcReader::initialise() {
    nfc.begin();
    this->version_data = nfc.getFirmwareVersion();
    if (!this->version_data) {
        return false;
    }
    nfc.SAMConfig();
    return true;
}

void NfcReader::stateful_communication() {
    if (!this->connected) {
        this->selected = false;
        this->check_connection();
    }

    if (this->connected && !this->selected)
        this->select_hce();

    if (this->connected && this->selected)
        this->communicate();
}