#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// Konstanten für das Protokoll
#define MAX_PACKET_SIZE 1024
#define HEADER_MAGIC 0xAA55
#define MIN_PACKET_SIZE 8  // Header + CRC
#define PAYLOAD_OFFSET 6

// CRC32 Tabelle
static uint32_t crc32_table[256];

// Packet Struktur
typedef struct {
    uint16_t magic;
    uint16_t length;
    uint16_t command;
    uint8_t payload[MAX_PACKET_SIZE - 8];
    uint32_t crc;
} IndustrialPacket;

// Parser Status
typedef enum {
    PARSER_WAIT_HEADER,
    PARSER_READ_LENGTH,
    PARSER_READ_COMMAND,
    PARSER_READ_PAYLOAD,
    PARSER_CHECK_CRC,
    PARSER_ERROR
} ParserState;

// Parser Kontext
typedef struct {
    ParserState state;
    IndustrialPacket current_packet;
    size_t bytes_received;
    uint8_t buffer[MAX_PACKET_SIZE];
} ParserContext;

// CRC32 Tabelle initialisieren
void init_crc32_table(void) {
    uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (size_t j = 0; j < 8; j++) {
            if (c & 1) {
                c = polynomial ^ (c >> 1);
            } else {
                c >>= 1;
            }
        }
        crc32_table[i] = c;
    }
}

// CRC32 berechnen
uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }
    
    return crc ^ 0xFFFFFFFF;
}

// Parser initialisieren
void parser_init(ParserContext *ctx) {
    if (!ctx) return;
    
    memset(ctx, 0, sizeof(ParserContext));
    ctx->state = PARSER_WAIT_HEADER;
}

// Packet validieren
bool validate_packet(const IndustrialPacket *packet, size_t received_length) {
    if (!packet) return false;
    
    // Basis-Validierungen
    if (packet->magic != HEADER_MAGIC) return false;
    if (packet->length < MIN_PACKET_SIZE || packet->length > MAX_PACKET_SIZE) return false;
    if (received_length != packet->length) return false;
    
    // CRC prüfen
    uint32_t calculated_crc = calculate_crc32((uint8_t*)packet, packet->length - sizeof(uint32_t));
    return calculated_crc == packet->crc;
}

// Hauptparser-Funktion
bool parse_packet(ParserContext *ctx, const uint8_t *data, size_t length) {
    if (!ctx || !data || length == 0) return false;
    
    size_t remaining_space = MAX_PACKET_SIZE - ctx->bytes_received;
    size_t bytes_to_copy = (length > remaining_space) ? remaining_space : length;
    
    // Puffer-Überlauf verhindern
    if (bytes_to_copy == 0) {
        ctx->state = PARSER_ERROR;
        return false;
    }
    
    // Daten in temporären Puffer kopieren
    memcpy(ctx->buffer + ctx->bytes_received, data, bytes_to_copy);
    ctx->bytes_received += bytes_to_copy;
    
    // State Machine
    bool packet_complete = false;
    while (!packet_complete) {
        switch (ctx->state) {
            case PARSER_WAIT_HEADER: {
                if (ctx->bytes_received < sizeof(uint16_t)) return false;
                
                uint16_t potential_magic;
                memcpy(&potential_magic, ctx->buffer, sizeof(uint16_t));
                
                if (potential_magic != HEADER_MAGIC) {
                    // Ungültiger Header - einen Byte verschieben und weitersuchen
                    memmove(ctx->buffer, ctx->buffer + 1, ctx->bytes_received - 1);
                    ctx->bytes_received--;
                    return false;
                }
                
                ctx->state = PARSER_READ_LENGTH;
                break;
            }
            
            case PARSER_READ_LENGTH: {
                if (ctx->bytes_received < 4) return false;
                
                uint16_t packet_length;
                memcpy(&packet_length, ctx->buffer + 2, sizeof(uint16_t));
                
                if (packet_length < MIN_PACKET_SIZE || packet_length > MAX_PACKET_SIZE) {
                    ctx->state = PARSER_ERROR;
                    return false;
                }
                
                ctx->current_packet.length = packet_length;
                ctx->state = PARSER_READ_COMMAND;
                break;
            }
            
            case PARSER_READ_COMMAND: {
                if (ctx->bytes_received < 6) return false;
                
                memcpy(&ctx->current_packet.command, ctx->buffer + 4, sizeof(uint16_t));
                ctx->state = PARSER_READ_PAYLOAD;
                break;
            }
            
            case PARSER_READ_PAYLOAD: {
                if (ctx->bytes_received < ctx->current_packet.length) return false;
                
                size_t payload_size = ctx->current_packet.length - MIN_PACKET_SIZE;
                memcpy(ctx->current_packet.payload, 
                       ctx->buffer + PAYLOAD_OFFSET, 
                       payload_size);
                
                ctx->state = PARSER_CHECK_CRC;
                break;
            }
            
            case PARSER_CHECK_CRC: {
                memcpy(&ctx->current_packet.crc, 
                       ctx->buffer + ctx->current_packet.length - sizeof(uint32_t),
                       sizeof(uint32_t));
                
                if (validate_packet(&ctx->current_packet, ctx->bytes_received)) {
                    packet_complete = true;
                } else {
                    ctx->state = PARSER_ERROR;
                    return false;
                }
                break;
            }
            
            case PARSER_ERROR:
            default:
                return false;
        }
    }
    
    // Reset für nächstes Packet
    ctx->state = PARSER_WAIT_HEADER;
    ctx->bytes_received = 0;
    
    return true;
}

// Beispiel für die Verwendung
void example_usage(void) {
    // Parser initialisieren
    ParserContext ctx;
    parser_init(&ctx);
    init_crc32_table();
    
    // Beispiel-Packet
    uint8_t test_packet[] = {
        0x55, 0xAA,  // Magic
        0x10, 0x00,  // Length (16 bytes)
        0x01, 0x00,  // Command
        0x01, 0x02, 0x03, 0x04,  // Payload
        0x78, 0x56, 0x34, 0x12   // CRC32
    };
    
    // Packet parsen
    if (parse_packet(&ctx, test_packet, sizeof(test_packet))) {
        // Packet erfolgreich geparst
        // Hier können die Daten weiterverarbeitet werden
    }
}