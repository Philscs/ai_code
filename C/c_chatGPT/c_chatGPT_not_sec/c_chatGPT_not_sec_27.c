#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Definition der Paketstruktur
#define MAX_PAYLOAD_SIZE 64

typedef struct {
    uint8_t header;        // 1 Byte Header
    uint8_t command;       // 1 Byte Kommando
    uint8_t payload_length; // 1 Byte Nutzlastlänge
    uint8_t payload[MAX_PAYLOAD_SIZE]; // Nutzlast
    uint8_t checksum;      // 1 Byte Prüfsumme
} Packet;

// Funktionen zur Berechnung der Checksumme
uint8_t calculate_checksum(Packet *packet) {
    uint8_t checksum = packet->header ^ packet->command ^ packet->payload_length;
    for (uint8_t i = 0; i < packet->payload_length; i++) {
        checksum ^= packet->payload[i];
    }
    return checksum;
}

// Funktion zum Senden eines Pakets
void send_packet(Packet *packet) {
    packet->checksum = calculate_checksum(packet);

    // Beispielhafte Ausgabe
    printf("Sending Packet:\n");
    printf("Header: 0x%02X\n", packet->header);
    printf("Command: 0x%02X\n", packet->command);
    printf("Payload Length: %d\n", packet->payload_length);
    printf("Payload: ");
    for (uint8_t i = 0; i < packet->payload_length; i++) {
        printf("0x%02X ", packet->payload[i]);
    }
    printf("\nChecksum: 0x%02X\n", packet->checksum);
}

// Funktion zum Empfangen eines Pakets
int receive_packet(Packet *packet) {
    uint8_t received_checksum = packet->checksum;
    uint8_t calculated_checksum = calculate_checksum(packet);

    if (received_checksum != calculated_checksum) {
        printf("Checksum mismatch! Packet is invalid.\n");
        return -1; // Fehler
    }

    printf("Packet received successfully.\n");
    return 0; // Erfolg
}

// Beispielhafter Einsatz
int main() {
    Packet packet;

    // Paket initialisieren
    packet.header = 0xAA; // Beispielheader
    packet.command = 0x01; // Beispielkommando
    packet.payload_length = 3; // Beispielnutzlastlänge
    packet.payload[0] = 0x10;
    packet.payload[1] = 0x20;
    packet.payload[2] = 0x30;

    // Paket senden
    send_packet(&packet);

    // Empfang simulieren
    if (receive_packet(&packet) == 0) {
        printf("Payload processing...\n");
        // Hier könnte weitere Verarbeitung der Nutzlast stattfinden
    }

    return 0;
}
