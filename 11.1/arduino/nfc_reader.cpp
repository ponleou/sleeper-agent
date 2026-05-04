#include "include/nfc_reader.hpp"

NfcReader::NfcReader(HardwareSerial &serial) : pn532hsu(serial), nfc(pn532hsu) {
    this->version_data = 0;
    this->connected = false;
    this->selected = false;
}

void NfcReader::check_connection() {
    // TODO: implement interrupt with startPassiveTargetIDDetection with ISR and interrupt on IRQ
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;
    this->connected = this->nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, 20, true);

    if (this->connected)
        Serial.print("Connected");
}

void NfcReader::select_hce() {
    uint8_t selectAID[] = {CLA_STANDARD,
                           INS_SELECT,
                           INS_SELECT_P1_AID,
                           INS_SELECT_P1_FIRST,
                           14,
                           0xF0,
                           0x73,
                           0x6C,
                           0x65,
                           0x65,
                           0x70,
                           0x65,
                           0x72,
                           0x20,
                           0x61,
                           0x67,
                           0x65,
                           0x6E,
                           0x74};
    uint8_t resLen = 255;
    uint8_t response[resLen];

    if (this->nfc.inDataExchange(selectAID, sizeof(selectAID), response, &resLen))
        this->selected = response[resLen - 2] == 0x90 && response[resLen - 1] == 0x00;
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


void NfcCommands::identify(PN532 nfc) {
    uint8_t command[] = {CLA_PROPRIETARY, INS_IDENTIFY, P_NULL, P_NULL, LE_IDENTITY};

    uint8_t resLen = 255;
    uint8_t response[resLen];
    if (nfc.inDataExchange(command, sizeof(command), response, &resLen)) {
        if (response[resLen - 2] == 0x90 && response[resLen - 1] == 0x00) {
            char deviceID[7];
            memcpy(deviceID, response, 6);
            deviceID[6] = '\0';
            Serial.println(deviceID);
        }
    }
}

void NfcReader::communicate() {
    NfcCommands::identify(this->nfc);
}