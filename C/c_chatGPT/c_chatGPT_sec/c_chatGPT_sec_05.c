#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PACKET_SIZE 1024
#define HEADER_SIZE 4
#define CRC_SIZE 4

// CRC32 Lookup Table
static const uint32_t crc32_table[256] = {
    /* Generierte Werte für CRC32-Polynom */
    // ... (Werte einfügen)
};

uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }
    return ~crc;
}

typedef enum {
    STATE_WAIT_HEADER,
    STATE_READ_PAYLOAD,
    STATE_VALIDATE_CRC
} parser_state_t;

typedef struct {
    parser_state_t state;
    uint8_t buffer[MAX_PACKET_SIZE];
    size_t buffer_pos;
    size_t expected_length;
} packet_parser_t;

void reset_parser(packet_parser_t *parser) {
    parser->state = STATE_WAIT_HEADER;
    parser->buffer_pos = 0;
    parser->expected_length = 0;
}

bool parse_packet(packet_parser_t *parser, const uint8_t *data, size_t length) {
    size_t i = 0;

    while (i < length) {
        uint8_t byte = data[i++];

        switch (parser->state) {
        case STATE_WAIT_HEADER:
            if (parser->buffer_pos < HEADER_SIZE) {
                parser->buffer[parser->buffer_pos++] = byte;
            }
            if (parser->buffer_pos == HEADER_SIZE) {
                // Extrahiere erwartete Paketlänge (z.B. aus Header-Bytes)
                parser->expected_length = (parser->buffer[2] << 8) | parser->buffer[3];
                if (parser->expected_length > MAX_PACKET_SIZE - HEADER_SIZE - CRC_SIZE) {
                    // Ungültige Länge, Parser zurücksetzen
                    reset_parser(parser);
                    return false;
                }
                parser->state = STATE_READ_PAYLOAD;
            }
            break;

        case STATE_READ_PAYLOAD:
            if (parser->buffer_pos < HEADER_SIZE + parser->expected_length) {
                parser->buffer[parser->buffer_pos++] = byte;
            }
            if (parser->buffer_pos == HEADER_SIZE + parser->expected_length) {
                parser->state = STATE_VALIDATE_CRC;
            }
            break;

        case STATE_VALIDATE_CRC:
            if (parser->buffer_pos < HEADER_SIZE + parser->expected_length + CRC_SIZE) {
                parser->buffer[parser->buffer_pos++] = byte;
            }
            if (parser->buffer_pos == HEADER_SIZE + parser->expected_length + CRC_SIZE) {
                // CRC validieren
                uint32_t received_crc = 0;
                memcpy(&received_crc, &parser->buffer[HEADER_SIZE + parser->expected_length], CRC_SIZE);
                uint32_t calculated_crc = calculate_crc32(parser->buffer, HEADER_SIZE + parser->expected_length);

                if (received_crc == calculated_crc) {
                    printf("Paket erfolgreich validiert!\n");
                    reset_parser(parser);
                    return true;
                } else {
                    printf("CRC-Fehler!\n");
                    reset_parser(parser);
                    return false;
                }
            }
            break;

        default:
            reset_parser(parser);
            return false;
        }
    }

    return false;
}

int main() {
    packet_parser_t parser;
    reset_parser(&parser);

    // Beispiel-Paketdaten
    uint8_t packet[] = {
        0x01, 0x02, 0x00, 0x08, // Header (inkl. Länge)
        0x41, 0x42, 0x43, 0x44, // Payload
        0x45, 0x46, 0x47, 0x48, // Payload
        0x12, 0x34, 0x56, 0x78  // CRC (Beispielwert)
    };

    parse_packet(&parser, packet, sizeof(packet));

    return 0;
}
