#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// Constants for protocol parsing
#define MAX_BUFFER_SIZE 1024
#define MAX_PACKET_SIZE 256
#define PROTOCOL_HEADER 0xAA
#define PROTOCOL_FOOTER 0xFF
#define MAX_PAYLOAD_SIZE (MAX_PACKET_SIZE - 4)  // Header + Length + Footer + Checksum

// Protocol States
typedef enum {
    STATE_IDLE,
    STATE_HEADER_RECEIVED,
    STATE_LENGTH_RECEIVED,
    STATE_RECEIVING_PAYLOAD,
    STATE_CHECKSUM_RECEIVED,
    STATE_FOOTER_RECEIVED,
    STATE_ERROR
} ParserState;

// Log Levels
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

// Protocol Packet Structure
typedef struct {
    uint8_t header;
    uint8_t length;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t checksum;
    uint8_t footer;
} ProtocolPacket;

// Parser Context Structure
typedef struct {
    ParserState state;
    uint8_t buffer[MAX_BUFFER_SIZE];
    size_t buffer_index;
    size_t payload_index;
    ProtocolPacket current_packet;
    FILE* log_file;
    LogLevel log_level;
} ParserContext;

// Function prototypes
void parser_init(ParserContext* ctx);
void parser_reset(ParserContext* ctx);
bool parser_process_byte(ParserContext* ctx, uint8_t byte);
void log_message(ParserContext* ctx, LogLevel level, const char* message);
bool validate_packet(const ProtocolPacket* packet);
uint8_t calculate_checksum(const uint8_t* data, size_t length);

// Initialize parser context
void parser_init(ParserContext* ctx) {
    memset(ctx, 0, sizeof(ParserContext));
    ctx->state = STATE_IDLE;
    ctx->log_file = fopen("protocol_parser.log", "a");
    ctx->log_level = LOG_INFO;
    log_message(ctx, LOG_INFO, "Parser initialized");
}

// Reset parser state
void parser_reset(ParserContext* ctx) {
    ctx->state = STATE_IDLE;
    ctx->buffer_index = 0;
    ctx->payload_index = 0;
    memset(&ctx->current_packet, 0, sizeof(ProtocolPacket));
    log_message(ctx, LOG_DEBUG, "Parser reset");
}

// Logging system implementation
void log_message(ParserContext* ctx, LogLevel level, const char* message) {
    if (level >= ctx->log_level && ctx->log_file != NULL) {
        time_t now;
        time(&now);
        char* timestamp = ctime(&now);
        timestamp[strlen(timestamp) - 1] = '\0';  // Remove newline

        const char* level_str[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
        fprintf(ctx->log_file, "[%s] %s: %s\n", timestamp, level_str[level], message);
        fflush(ctx->log_file);
    }
}

// Calculate checksum for security validation
uint8_t calculate_checksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

// Validate packet for security
bool validate_packet(const ProtocolPacket* packet) {
    // Validate header and footer
    if (packet->header != PROTOCOL_HEADER || packet->footer != PROTOCOL_FOOTER) {
        return false;
    }

    // Validate length
    if (packet->length > MAX_PAYLOAD_SIZE) {
        return false;
    }

    // Validate checksum
    uint8_t calculated_checksum = calculate_checksum(packet->payload, packet->length);
    return calculated_checksum == packet->checksum;
}

// Main parser function - implements state machine
bool parser_process_byte(ParserContext* ctx, uint8_t byte) {
    char log_buffer[100];

    switch (ctx->state) {
        case STATE_IDLE:
            if (byte == PROTOCOL_HEADER) {
                ctx->current_packet.header = byte;
                ctx->state = STATE_HEADER_RECEIVED;
                log_message(ctx, LOG_DEBUG, "Header received");
            }
            break;

        case STATE_HEADER_RECEIVED:
            if (byte <= MAX_PAYLOAD_SIZE) {
                ctx->current_packet.length = byte;
                ctx->state = STATE_LENGTH_RECEIVED;
                snprintf(log_buffer, sizeof(log_buffer), "Length received: %d", byte);
                log_message(ctx, LOG_DEBUG, log_buffer);
            } else {
                log_message(ctx, LOG_ERROR, "Invalid length received");
                parser_reset(ctx);
            }
            break;

        case STATE_LENGTH_RECEIVED:
        case STATE_RECEIVING_PAYLOAD:
            if (ctx->payload_index < ctx->current_packet.length) {
                ctx->current_packet.payload[ctx->payload_index++] = byte;
                if (ctx->payload_index == ctx->current_packet.length) {
                    ctx->state = STATE_RECEIVING_PAYLOAD;
                }
            }
            if (ctx->payload_index == ctx->current_packet.length) {
                ctx->state = STATE_CHECKSUM_RECEIVED;
                log_message(ctx, LOG_DEBUG, "Payload received");
            }
            break;

        case STATE_CHECKSUM_RECEIVED:
            ctx->current_packet.checksum = byte;
            ctx->state = STATE_FOOTER_RECEIVED;
            log_message(ctx, LOG_DEBUG, "Checksum received");
            break;

        case STATE_FOOTER_RECEIVED:
            if (byte == PROTOCOL_FOOTER) {
                ctx->current_packet.footer = byte;
                
                // Validate packet
                if (validate_packet(&ctx->current_packet)) {
                    log_message(ctx, LOG_INFO, "Valid packet received");
                    // Process the valid packet here
                    parser_reset(ctx);
                    return true;
                } else {
                    log_message(ctx, LOG_ERROR, "Invalid packet - checksum mismatch");
                    parser_reset(ctx);
                }
            } else {
                log_message(ctx, LOG_ERROR, "Invalid footer received");
                parser_reset(ctx);
            }
            break;

        case STATE_ERROR:
            // Error recovery
            parser_reset(ctx);
            log_message(ctx, LOG_WARNING, "Recovering from error state");
            break;
    }

    return false;
}

// Example usage
int main() {
    ParserContext ctx;
    parser_init(&ctx);

    // Example packet
    uint8_t test_packet[] = {
        PROTOCOL_HEADER,
        0x03,               // Length
        0x01, 0x02, 0x03,  // Payload
        0x00,              // Checksum (to be calculated)
        PROTOCOL_FOOTER
    };

    // Calculate and set checksum
    test_packet[5] = calculate_checksum(&test_packet[2], 3);

    // Process packet byte by byte
    for (size_t i = 0; i < sizeof(test_packet); i++) {
        parser_process_byte(&ctx, test_packet[i]);
    }

    // Cleanup
    if (ctx.log_file) {
        fclose(ctx.log_file);
    }

    return 0;
}