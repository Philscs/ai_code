#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define LOG_FILE "protocol_parser.log"

// Protocol States
typedef enum {
    STATE_IDLE,
    STATE_RECEIVING,
    STATE_PROCESSING,
    STATE_ERROR
} ProtocolState;

// Error Codes
typedef enum {
    ERR_NONE,
    ERR_INVALID_HEADER,
    ERR_CHECKSUM_FAILED,
    ERR_UNKNOWN_COMMAND
} ErrorCode;

// Protocol Parser Structure
typedef struct {
    ProtocolState state;
    uint8_t buffer[BUFFER_SIZE];
    size_t buffer_pos;
    ErrorCode last_error;
    FILE *log_file;
} ProtocolParser;

// Prototypes
void log_message(ProtocolParser *parser, const char *format, ...);
bool validate_message(uint8_t *data, size_t length);
void handle_message(ProtocolParser *parser, uint8_t *data, size_t length);
void reset_parser(ProtocolParser *parser);
void process_data(ProtocolParser *parser, uint8_t *data, size_t length);

// Logging Function
void log_message(ProtocolParser *parser, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(parser->log_file, format, args);
    fprintf(parser->log_file, "\n");
    va_end(args);
    fflush(parser->log_file);
}

// Security Validation (e.g., checksum)
bool validate_message(uint8_t *data, size_t length) {
    if (length < 2) return false; // Minimum size check
    uint8_t checksum = 0;
    for (size_t i = 0; i < length - 1; ++i) {
        checksum ^= data[i];
    }
    return checksum == data[length - 1];
}

// Message Handler
void handle_message(ProtocolParser *parser, uint8_t *data, size_t length) {
    if (!validate_message(data, length)) {
        log_message(parser, "Message validation failed.");
        parser->last_error = ERR_CHECKSUM_FAILED;
        parser->state = STATE_ERROR;
        return;
    }

    log_message(parser, "Valid message received. Length: %zu", length);

    // Process command (Example: First byte is command ID)
    switch (data[0]) {
        case 0x01:
            log_message(parser, "Command 0x01 executed.");
            break;
        case 0x02:
            log_message(parser, "Command 0x02 executed.");
            break;
        default:
            log_message(parser, "Unknown command: 0x%02X", data[0]);
            parser->last_error = ERR_UNKNOWN_COMMAND;
            parser->state = STATE_ERROR;
            break;
    }
}

// Reset Parser
void reset_parser(ProtocolParser *parser) {
    parser->state = STATE_IDLE;
    parser->buffer_pos = 0;
    parser->last_error = ERR_NONE;
    log_message(parser, "Parser reset.");
}

// Process Incoming Data
void process_data(ProtocolParser *parser, uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte = data[i];

        switch (parser->state) {
            case STATE_IDLE:
                if (byte == 0xAA) { // Example: Start of Message
                    parser->buffer_pos = 0;
                    parser->state = STATE_RECEIVING;
                    log_message(parser, "Start of message detected.");
                }
                break;

            case STATE_RECEIVING:
                if (parser->buffer_pos < BUFFER_SIZE) {
                    parser->buffer[parser->buffer_pos++] = byte;
                    if (byte == 0xFF) { // Example: End of Message
                        parser->state = STATE_PROCESSING;
                        log_message(parser, "End of message detected.");
                    }
                } else {
                    log_message(parser, "Buffer overflow.");
                    parser->last_error = ERR_INVALID_HEADER;
                    parser->state = STATE_ERROR;
                }
                break;

            case STATE_PROCESSING:
                handle_message(parser, parser->buffer, parser->buffer_pos);
                reset_parser(parser);
                break;

            case STATE_ERROR:
                log_message(parser, "Error state. Resetting parser.");
                reset_parser(parser);
                break;
        }
    }
}

int main() {
    ProtocolParser parser = {
        .state = STATE_IDLE,
        .buffer_pos = 0,
        .last_error = ERR_NONE,
        .log_file = fopen(LOG_FILE, "w")
    };

    if (!parser.log_file) {
        fprintf(stderr, "Failed to open log file.\n");
        return 1;
    }

    uint8_t test_data[] = {0xAA, 0x01, 0x02, 0x03, 0x00, 0xFF};
    process_data(&parser, test_data, sizeof(test_data));

    fclose(parser.log_file);
    return 0;
}
