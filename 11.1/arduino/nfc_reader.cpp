#include "include/nfc_reader.hpp"

NfcReader::NfcReader(HardwareSerial &serial, IBleHostWriter &host) : pn532hsu(serial), nfc(pn532hsu), host(host) {
    this->version_data = 0;
    this->last_communication_ms = millis();
    this->reset_state();
}

void NfcReader::check_connection() {
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;
    this->connected = this->nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, 20, true);
}

void NfcReader::select_hce() {
    this->selected = false;
    uint8_t selectAID[] = {CLA_STANDARD,
                           INS_SELECT,
                           INS_SELECT_P1_AID,
                           INS_SELECT_P2_FIRST,
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
    uint8_t response[255];
    uint8_t res_length = sizeof(response);

    if (this->nfc.inDataExchange(selectAID, sizeof(selectAID), response, &res_length))
        this->selected = res_length >= 2 && response[res_length - 2] == 0x90 && response[res_length - 1] == 0x00;
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
    this->identified = false;
    this->client_collector_active = false;
    this->collected_queue = {};

    this->identity = " ";
    host.set_session_id(this->identity);
    host.reset_enqueued();
    host.write_metadata(" ");
}

void NfcReader::stateful_communication() {
    if (millis() - this->last_communication_ms < POLLING_PERIOD)
        return;

    this->last_communication_ms = millis();

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

    this->communicate();
    bool written = this->host.enqueue_data(this->collected_queue);
    if (written) {
        Serial.println("Written");
    }
}

bool NfcCommands::identify(PN532 &nfc, String *value) {
    uint8_t command[] = {CLA_PROPRIETARY, INS_IDENTIFY, P_NULL, P_NULL, LE_ID_LENGTH};

    uint8_t response[255];
    uint8_t res_length = sizeof(response);
    if (!nfc.inDataExchange(command, sizeof(command), response, &res_length))
        return false;

    if (res_length < 2 || response[res_length - 2] != 0x90 || response[res_length - 1] != 0x00)
        return false;

    char deviceID[7];
    memcpy(deviceID, response, 6);
    deviceID[6] = '\0';

    *value = String(deviceID);
    Serial.println(deviceID);

    return true;
}

bool NfcCommands::poll(PN532 &nfc, bool *initiate_collect) {
    uint8_t command[] = {CLA_PROPRIETARY, INS_POLL, P_NULL, P_NULL};

    uint8_t response[255];
    uint8_t res_length = sizeof(response);
    if (!nfc.inDataExchange(command, sizeof(command), response, &res_length))
        return false;

    if (res_length < 2)
        return false;

    bool success = response[res_length - 2] == SW1_SUCCESS && response[res_length - 1] == SW2_SUCCESS;
    bool data = response[res_length - 2] == SW1_DATA;
    *initiate_collect = data;

    return success || data;
}

bool NfcCommands::collect(PN532 &nfc, bool *initiate_collect, uint8_t data[], uint8_t *data_length) {
    uint8_t command[] = {CLA_PROPRIETARY, INS_COLLECT, P1_COLLECT_PROVIDE_LENGTH, P2_COLLECT_LENGTH_POS, LE_ALL};

    uint8_t response[255];
    uint8_t res_length = sizeof(response);
    if (!nfc.inDataExchange(command, sizeof(command), response, &res_length))
        return false;

    if (res_length < 3)
        return false;

    bool success = response[res_length - 2] == SW1_SUCCESS && response[res_length - 1] == SW2_SUCCESS;
    bool has_more_data = response[res_length - 2] == SW1_DATA;
    *initiate_collect = has_more_data;

    if (!(success || has_more_data))
        return false;

    uint8_t length = response[P2_COLLECT_LENGTH_POS];
    *data_length = min(length, res_length - 3);
    memcpy(data, response + 1, *data_length);

    return success || has_more_data;
}

bool NfcCommands::start_client_collector(PN532 &nfc) {
    uint8_t command[] = {CLA_PROPRIETARY, INS_START_CLIENT_COLLECTOR, P_NULL, P_NULL};

    uint8_t response[255];
    uint8_t res_length = sizeof(response);
    if (!nfc.inDataExchange(command, sizeof(command), response, &res_length))
        return false;

    if (res_length < 2)
        return false;

    return response[res_length - 2] == SW1_SUCCESS && response[res_length - 1] == SW2_SUCCESS;
}

bool NfcCommands::stop_client_collector(PN532 &nfc) {
    uint8_t command[] = {CLA_PROPRIETARY, INS_STOP_CLIENT_COLLECTOR, P_NULL, P_NULL};

    uint8_t response[255];
    uint8_t res_length = sizeof(response);
    if (!nfc.inDataExchange(command, sizeof(command), response, &res_length))
        return false;

    if (res_length < 2)
        return false;

    return response[res_length - 2] == SW1_SUCCESS && response[res_length - 1] == SW2_SUCCESS;
}

bool NfcCommands::open_weblink(PN532 &nfc, String link) {
    uint8_t link_length = link.length();
    uint8_t command[5 + link_length];
    command[0] = CLA_PROPRIETARY;
    command[1] = INS_OPEN_WEBLINK;
    command[2] = P_NULL;
    command[3] = P_NULL;
    command[4] = link_length;
    memcpy(&command[5], link.c_str(), link_length);

    uint8_t response[255];
    uint8_t res_length = sizeof(response);
    if (!nfc.inDataExchange(command, sizeof(command), response, &res_length))
        return false;

    if (res_length < 2)
        return false;

    return response[res_length - 2] == SW1_SUCCESS && response[res_length - 1] == SW2_SUCCESS;
}

bool NfcCommands::collect_metadata(PN532 &nfc, String *value) {
    uint8_t command[] = {CLA_PROPRIETARY, INS_COLLECT_META, P_NULL, P_NULL, LE_ALL};

    uint8_t response[255];
    uint8_t res_length = sizeof(response);
    if (!nfc.inDataExchange(command, sizeof(command), response, &res_length))
        return false;

    if (res_length < 2 || response[res_length - 2] != 0x90 || response[res_length - 1] != 0x00)
        return false;

    String result = "";
    for (int i = 0; i < res_length - 2; i++) {
        if (response[i] == '\0') {
            result += '|';
        } else if (response[i] != '|') {
            result += (char)response[i];
        }
    }
    *value = result;
    return true;
}

void NfcReader::communicate() {
    if (!this->identified) {
        this->identified = this->connected = NfcCommands::identify(this->nfc, &this->identity);
        if (this->identified)
            host.set_session_id(this->identity);
    }

    if (!(this->connected && this->identified))
        return;

    String metadata = "";
    this->connected = NfcCommands::collect_metadata(this->nfc, &metadata);
    if (!this->connected)
        return;
    if (metadata != "") {
        this->host.write_metadata(metadata);
        Serial.println(metadata);
    }

    // this->connected = NfcCommands::open_weblink(this->nfc, "google.com");
    // if (!this->connected) {
    //     Serial.println("failed to open link");
    //     return;
    // }

    if (!this->client_collector_active) {
        this->client_collector_active = this->connected = NfcCommands::start_client_collector(this->nfc);
        if (!this->connected)
            return;
    }

    bool collect_data = false;
    this->connected = NfcCommands::poll(this->nfc, &collect_data);
    if (!this->connected)
        return;

    uint8_t data_length = 0;
    uint8_t data[255];
    while (collect_data && this->connected) {
        collect_data = false;
        this->connected = NfcCommands::collect(this->nfc, &collect_data, data, &data_length);

        if (data_length > 0) {
            String combined = "";
            String segment = "";

            // replace \0 delimitter with |, while also removing all | from the data itself
            for (uint8_t i = 0; i < data_length; i++) {
                if (data[i] == '\0') {
                    segment.replace("|", "");
                    if (combined.length() > 0)
                        combined += "|";
                    combined += segment;
                    segment = "";
                } else {
                    segment += (char)data[i];
                }
            }

            // for the last completed segment (where it doesnt find a \0)
            if (segment.length() > 0) {
                segment.replace("|", "");
                if (combined.length() > 0)
                    combined += "|";
                combined += segment;
            }

            this->collected_queue.push(combined);
            Serial.println(combined);
        }
    }
}

void NfcReader::start_action() {

}