// iot_protocol.h
#ifndef IOT_PROTOCOL_H
#define IOT_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// Konfigurierbare Parameter
#define MAX_PACKET_SIZE 128
#define MAX_BUFFER_SIZE 256
#define MAX_RETRIES 3
#define TIMEOUT_MS 1000

// Pakettypen
typedef enum {
    PACKET_TYPE_DATA = 0x01,
    PACKET_TYPE_ACK = 0x02,
    PACKET_TYPE_NACK = 0x03,
    PACKET_TYPE_PING = 0x04
} PacketType;

// Paketstruktur
typedef struct {
    uint8_t type;
    uint8_t seq_num;
    uint8_t length;
    uint8_t data[MAX_PACKET_SIZE];
    uint16_t checksum;
} Packet;

// Protocol Stack Kontext
typedef struct {
    uint8_t rx_buffer[MAX_BUFFER_SIZE];
    uint8_t tx_buffer[MAX_BUFFER_SIZE];
    uint8_t current_seq;
    bool is_initialized;
} ProtocolContext;

// Funktionsprototypen
bool protocol_init(ProtocolContext* ctx);
bool protocol_send(ProtocolContext* ctx, const uint8_t* data, uint8_t length);
bool protocol_receive(ProtocolContext* ctx, uint8_t* data, uint8_t* length);
void protocol_cleanup(ProtocolContext* ctx);

#endif // IOT_PROTOCOL_H

// iot_protocol.c
#include "iot_protocol.h"
#include <string.h>

// CRC16 Lookup-Tabelle für effiziente Checksummen-Berechnung
static const uint16_t crc16_table[256] = {
    // ... (CRC16-CCITT Tabelle hier einfügen)
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7
};

// Hilfsfunktionen
static uint16_t calculate_checksum(const uint8_t* data, uint8_t length) {
    uint16_t crc = 0xFFFF;
    
    for (uint8_t i = 0; i < length; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }
    
    return crc;
}

static bool validate_packet(const Packet* packet) {
    uint16_t calc_checksum = calculate_checksum(packet->data, packet->length);
    return calc_checksum == packet->checksum;
}

// Protocol Stack Initialisierung
bool protocol_init(ProtocolContext* ctx) {
    if (!ctx) return false;
    
    memset(ctx->rx_buffer, 0, MAX_BUFFER_SIZE);
    memset(ctx->tx_buffer, 0, MAX_BUFFER_SIZE);
    ctx->current_seq = 0;
    ctx->is_initialized = true;
    
    return true;
}

// Paket senden mit Wiederholungsversuch
bool protocol_send(ProtocolContext* ctx, const uint8_t* data, uint8_t length) {
    if (!ctx || !ctx->is_initialized || !data || length > MAX_PACKET_SIZE) {
        return false;
    }
    
    Packet packet = {
        .type = PACKET_TYPE_DATA,
        .seq_num = ctx->current_seq,
        .length = length
    };
    
    memcpy(packet.data, data, length);
    packet.checksum = calculate_checksum(packet.data, length);
    
    // Hier würde die eigentliche Übertragung erfolgen
    // Implementierung abhängig von der Hardware-Schnittstelle
    
    ctx->current_seq = (ctx->current_seq + 1) % 256;
    return true;
}

// Paket empfangen mit Fehlerprüfung
bool protocol_receive(ProtocolContext* ctx, uint8_t* data, uint8_t* length) {
    if (!ctx || !ctx->is_initialized || !data || !length) {
        return false;
    }
    
    // Hier würde der eigentliche Empfang erfolgen
    // Implementierung abhängig von der Hardware-Schnittstelle
    
    Packet received_packet;
    // Beispiel für Paketempfang und Validierung
    if (!validate_packet(&received_packet)) {
        return false;
    }
    
    memcpy(data, received_packet.data, received_packet.length);
    *length = received_packet.length;
    
    // ACK senden
    Packet ack_packet = {
        .type = PACKET_TYPE_ACK,
        .seq_num = received_packet.seq_num,
        .length = 0
    };
    
    // ACK-Paket senden
    // Implementierung abhängig von der Hardware-Schnittstelle
    
    return true;
}

// Aufräumen und Ressourcen freigeben
void protocol_cleanup(ProtocolContext* ctx) {
    if (!ctx) return;
    
    memset(ctx->rx_buffer, 0, MAX_BUFFER_SIZE);
    memset(ctx->tx_buffer, 0, MAX_BUFFER_SIZE);
    ctx->is_initialized = false;
}