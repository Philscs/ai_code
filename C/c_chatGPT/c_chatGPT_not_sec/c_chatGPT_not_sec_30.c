#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Definition von Konstanten
#define PACKET_BUFFER_SIZE 2048
#define MAX_PACKET_QUEUE 256

// DMA-Ringstruktur
struct dma_descriptor {
    void *buffer;
    uint32_t length;
    uint32_t status;
};

// Paketstruktur
struct packet {
    void *data;
    uint32_t length;
};

// Netzwerkgeräte-Struktur
struct network_device {
    struct dma_descriptor *rx_ring;
    struct dma_descriptor *tx_ring;
    size_t rx_ring_size;
    size_t tx_ring_size;
    size_t rx_head;
    size_t tx_head;
    size_t tx_tail;
};

// Initialisierung des Netzwerkgeräts
struct network_device *initialize_device(size_t rx_ring_size, size_t tx_ring_size) {
    struct network_device *dev = malloc(sizeof(struct network_device));
    if (!dev) {
        perror("Gerät-Speicher konnte nicht zugewiesen werden");
        return NULL;
    }

    dev->rx_ring_size = rx_ring_size;
    dev->tx_ring_size = tx_ring_size;
    dev->rx_head = 0;
    dev->tx_head = 0;
    dev->tx_tail = 0;

    // Speicher für RX- und TX-Ringe allokieren
    dev->rx_ring = calloc(rx_ring_size, sizeof(struct dma_descriptor));
    dev->tx_ring = calloc(tx_ring_size, sizeof(struct dma_descriptor));

    if (!dev->rx_ring || !dev->tx_ring) {
        perror("DMA-Ring-Speicher konnte nicht zugewiesen werden");
        free(dev);
        return NULL;
    }

    // RX-Puffer vorbereiten
    for (size_t i = 0; i < rx_ring_size; i++) {
        dev->rx_ring[i].buffer = malloc(PACKET_BUFFER_SIZE);
        dev->rx_ring[i].length = PACKET_BUFFER_SIZE;
        dev->rx_ring[i].status = 0; // Markiert als bereit
    }

    return dev;
}

// Empfang eines Pakets
struct packet *receive_packet(struct network_device *dev) {
    struct dma_descriptor *desc = &dev->rx_ring[dev->rx_head];

    if (desc->status == 0) {
        return NULL; // Kein Paket verfügbar
    }

    struct packet *pkt = malloc(sizeof(struct packet));
    if (!pkt) {
        perror("Paket-Speicher konnte nicht zugewiesen werden");
        return NULL;
    }

    pkt->data = desc->buffer;
    pkt->length = desc->length;

    // DMA-Descriptor zurücksetzen
    desc->buffer = malloc(PACKET_BUFFER_SIZE);
    desc->length = PACKET_BUFFER_SIZE;
    desc->status = 0; // Wieder bereit

    dev->rx_head = (dev->rx_head + 1) % dev->rx_ring_size;

    return pkt;
}

// Senden eines Pakets
bool send_packet(struct network_device *dev, struct packet *pkt) {
    struct dma_descriptor *desc = &dev->tx_ring[dev->tx_tail];

    if (desc->status != 0) {
        return false; // Kein Platz im Ring
    }

    desc->buffer = pkt->data;
    desc->length = pkt->length;
    desc->status = 1; // Markiert als in Verwendung

    dev->tx_tail = (dev->tx_tail + 1) % dev->tx_ring_size;

    return true;
}

// Freigabe der Ressourcen
void cleanup_device(struct network_device *dev) {
    for (size_t i = 0; i < dev->rx_ring_size; i++) {
        free(dev->rx_ring[i].buffer);
    }
    for (size_t i = 0; i < dev->tx_ring_size; i++) {
        if (dev->tx_ring[i].buffer) {
            free(dev->tx_ring[i].buffer);
        }
    }
    free(dev->rx_ring);
    free(dev->tx_ring);
    free(dev);
}

int main() {
    // Initialisierung
    struct network_device *dev = initialize_device(128, 128);
    if (!dev) {
        return EXIT_FAILURE;
    }

    // Simuliertes Paket senden
    struct packet pkt = {"Testdaten", 9};
    if (!send_packet(dev, &pkt)) {
        fprintf(stderr, "Senden des Pakets fehlgeschlagen\n");
    }

    // Ressourcen freigeben
    cleanup_device(dev);

    return EXIT_SUCCESS;
}
