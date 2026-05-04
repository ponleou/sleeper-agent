#include "include/nfc_reader.hpp"

NfcReader::NfcReader(HardwareSerial &serial) : pn532hsu(serial), nfc(pn532hsu) {
    this->version_data = 0;
    this->last_communication_ms = millis();
    this->reset_state();
}

void NfcReader::check_connection() {
    // TODO: implement interrupt with startPassiveTargetIDDetection with ISR and interrupt on IRQ
    // probably not in this function, because right now, it is purely for checking conection (used in communicatio after
    // complete)
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;
    this->connected = this->nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, 20, true);

    // if (this->connected)
    //     Serial.print("Connected");
}

void NfcReader::check_persistent_connection(uint8_t tolerance = 5) {
    if(NfcCommands::poll(this->nfc))
        this->failed_connection_count = 0;
    else
        this->failed_connection_count++;

    if (this->failed_connection_count > tolerance)
        this->reset_state();
}

void NfcReader::select_hce() {
    this->selected = false;
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
        this->selected = resLen >= 2 && response[resLen - 2] == 0x90 && response[resLen - 1] == 0x00;
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

void NfcReader::reset_state() {
    this->connected = false;
    this->selected = false;
    this->completed = false;
    this->failed_connection_count = 0;
}

void NfcReader::stateful_communication() {
    if (millis() - this->last_communication_ms < 500)
        return;

    this->last_communication_ms = millis();

    char status[32] = {0};
    if (this->connected)
        strcat(status, "Connected ");
    if (this->selected)
        strcat(status, "Selected ");
    if (this->completed)
        strcat(status, "Completed ");
    Serial.println(status);

    if (!this->connected) {
        this->reset_state();
        this->check_connection();
    }

    if (!this->connected)
        return;

    if (!this->selected)
        this->select_hce();

    if (!this->selected) {
        this->reset_state();
        return;
    }

    if (!this->completed)
        this->communicate();
    else 
        this->check_persistent_connection();
}

bool NfcCommands::identify(PN532 &nfc) {
    uint8_t command[] = {CLA_PROPRIETARY, INS_IDENTIFY, P_NULL, P_NULL, LE_IDENTITY};

    uint8_t resLen = 255;
    uint8_t response[resLen];
    if (!nfc.inDataExchange(command, sizeof(command), response, &resLen))
        return false;

    if (resLen < 2 || response[resLen - 2] != 0x90 || response[resLen - 1] != 0x00)
        return false;

    char deviceID[7];
    memcpy(deviceID, response, 6);
    deviceID[6] = '\0';
    Serial.println(deviceID);
    return true;
}

bool NfcCommands::poll(PN532 &nfc) {
    uint8_t command[] = {CLA_PROPRIETARY, INS_POLL, P_NULL, P_NULL};

    uint8_t resLen = 255;
    uint8_t response[resLen];
    if (!nfc.inDataExchange(command, sizeof(command), response, &resLen))
        return false;

    if (resLen < 2 || response[resLen - 2] != 0x90 || response[resLen - 1] != 0x00)
        return false;

    return true;
}

void NfcReader::communicate() {
    if (!this->completed)
        this->connected = NfcCommands::identify(this->nfc);

    if (!this->connected)
        return;

    this->completed = true;
}