#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Foramen Header
typedef struct {
    uint32_t src_port;
    uint16_t dst_port;
    uint8_t f_type; // F-type (FCS, DCS, FCIP)
    uint16_t seq_num; // Sequence Number
    uint32_t crc32;  // CRC32-Checksum
} foramen_header;

// Foramen State Machine
typedef enum {
    FORAMEM_STATE_INVALID,
    FORAMEM_STATE_HEADER_VALID,
    FORAMEM_STATE_DATA_VALID,
    FORAMEM_STATE_ERROR
} foramen_state;

void foramen_parse_packet(uint8_t *packet, size_t packet_len, foramen_state *state) {
    // State Initialization
    *state = FORAMEM_STATE_INVALID;
    bool header_valid = true;
    uint32_t crc32_calculated = 0;

    // Header Parsen
    if (packet_len >= sizeof(foramen_header)) {
        foramen_header header = *(foramen_header*)packet;
        packet += sizeof(foramen_header);

        // CRC32-Prüfung
        crc32_calculated = crc32(packet, packet_len - sizeof(foramen_header));

        // Foramen-Typprüfung
        switch (header.f_type) {
            case FORAMEM_FCS:
                header_valid = true;
                break;
            case FORAMEM_DCS:
                header_valid = false;
                break;
            default:
                header_valid = false;
                break;
        }

        // Sequence-Nummerprüfung
        if (header.seq_num > 0xFFFF) {
            header_valid = false;
        }
    } else {
        header_valid = false;
    }

    *state = FORAMEM_STATE_HEADER_VALID;

    if (header_valid) {
        // Daten parsen
        uint8_t data_packet[packet_len - sizeof(foramen_header)];
        memcpy(data_packet, packet + sizeof(foramen_header), packet_len - 
sizeof(foramen_header));
        *state = FORAMEM_STATE_DATA_VALID;
    } else {
        *state = FORAMEM_STATE_ERROR;
    }
}

// CRC32-Prüfungsfunktion
uint32_t crc32(uint8_t *data, size_t data_len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < data_len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if ((crc & 1) != 0) {
                crc >>= 1;
                crc ^= 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

int main() {
    uint8_t packet[] = {
        // Header
        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x00, 0x11,
        // Daten
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    };

    size_t packet_len = sizeof(packet);
    foramen_state state;
    foramen_parse_packet(packet, packet_len, &state);

    switch (state) {
        case FORAMEM_STATE_INVALID:
            printf("Invalid Foramen Packet\n");
            break;
        case FORAMEM_STATE_HEADER_VALID:
            printf("Foramen Header Valid\n");
            break;
        case FORAMEM_STATE_DATA_VALID:
            printf("Foramen Daten valid\n");
            break;
        case FORAMEM_STATE_ERROR:
            printf("Foramen Fehler\n");
            break;
    }

    return 0;
}
