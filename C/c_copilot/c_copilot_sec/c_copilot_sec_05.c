#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the states of the packet parser state machine
typedef enum {
    STATE_IDLE,
    STATE_HEADER,
    STATE_PAYLOAD,
    STATE_CRC,
    STATE_VALIDATION_SUCCESS,
    STATE_VALIDATION_FAILURE
} ParserState;

// Function to calculate the CRC32 checksum of a string
unsigned int calculateCRC32(const char* str) {
    unsigned int crc = 0xFFFFFFFF;
    int i, j;

    for (i = 0; str[i] != '\0'; i++) {
        crc ^= str[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

// Function to parse and validate a network packet
void parsePacket(const char* packet) {
    ParserState state = STATE_IDLE;
    int packetLength = strlen(packet);

    for (int i = 0; i < packetLength; i++) {
        switch (state) {
            case STATE_IDLE:
                if (packet[i] == 'H') {
                    state = STATE_HEADER;
                }
                break;

            case STATE_HEADER:
                if (packet[i] == 'P') {
                    state = STATE_PAYLOAD;
                } else {
                    state = STATE_IDLE;
                }
                break;

            case STATE_PAYLOAD:
                if (packet[i] == 'C') {
                    state = STATE_CRC;
                }
                break;

            case STATE_CRC:
                if (packet[i] == 'V') {
                    unsigned int crc32 = calculateCRC32(packet);
                    if (crc32 == 0) {
                        state = STATE_VALIDATION_SUCCESS;
                    } else {
                        state = STATE_VALIDATION_FAILURE;
                    }
                } else {
                    state = STATE_VALIDATION_FAILURE;
                }
                break;

            default:
                break;
        }
    }

    if (state == STATE_VALIDATION_SUCCESS) {
        printf("Packet validation successful.\n");
    } else {
        printf("Packet validation failed.\n");
    }
}

int main() {
    const char* packet = "HPayloadCRCV";
    parsePacket(packet);

    return 0;
}
