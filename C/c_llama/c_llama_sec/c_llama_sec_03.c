#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Definition der Struktur für das Binärformat
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint8_t  checksum[32];
} file_header;

// Funktion zum Parsen eines Files
file_header parse_file(const uint8_t* buffer, size_t length) {
    // Überprüfung der Länge des Buffers
    if (length < sizeof(file_header)) {
        return (file_header){0};
    }

    // Lesen des File-Headers
    file_header header = *reinterpret_cast<const file_header*>(buffer);

    // Überprüfung der Magic-Nummer
    if (header.magic != 0x12345678) { // Beispiel-Wert, kann je nach Format geändert werden
        return (file_header){0};
    }

    // Überprüfung der Version
    if (header.version < 1 || header.version > 10) {
        return (file_header){0};
    }

    // Überprüfung des Data-Size
    if (header.data_size > length - sizeof(file_header)) {
        return (file_header){0};
    }

    // Überprüfung der Checksum
    uint32_t calculated_checksum = 0;
    for (size_t i = 0; i < header.data_size; ++i) {
        calculated_checksum += buffer[i + sizeof(file_header)];
    }
    if (calculated_checksum != header.checksum[0]) { // 32-Bit-Checksum, beginnend mit der ersten 
Byte
        return (file_header){0};
    }

    // Überprüfung auf Integer Overflow bei der Version
    if (header.version > UINT32_MAX - 1) {
        printf("Integer Overflow: Version ist zu groß (%u)\n", header.version);
        return (file_header){0};
    }

    // Wenn alle Überprüfungen erfolgreich waren, gibt das Parser die Header-Struktur zurück
    return header;
}

int main() {
    const uint8_t buffer[] = {
        0x12, 0x34, 0x56, 0x78, // Magic
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, // Version
        sizeof(file_header) - 1, // Data-Size
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, // Checksum
    };

    file_header header = parse_file(buffer, sizeof(buffer));
    if (header.magic == 0) {
        printf("Parsiert nicht erfolgreich.\n");
    } else {
        printf("Parsiert erfolgreich:\n");
        printf("Magic: 0x%08x\n", header.magic);
        printf("Version: %u\n", header.version);
        printf("Data-Size: %u\n", header.data_size);
        for (size_t i = 0; i < sizeof(header.checksum); ++i) {
            printf("%02x ", header.checksum[i]);
        }
        printf("\n");
    }

    return 0;
}
