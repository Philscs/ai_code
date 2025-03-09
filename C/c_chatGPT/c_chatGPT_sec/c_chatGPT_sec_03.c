#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Definition der Datei-Header-Struktur
struct file_header {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint8_t checksum[32];
};

// Konstante für den erwarteten Magic-Wert
#define EXPECTED_MAGIC 0x12345678

// Hilfsfunktion: Sicheres Lesen von Daten aus einer Datei
size_t safe_read(void *buffer, size_t size, size_t count, FILE *stream) {
    size_t bytesRead = fread(buffer, size, count, stream);
    if (bytesRead != count && ferror(stream)) {
        perror("Fehler beim Lesen der Datei");
        exit(EXIT_FAILURE);
    }
    return bytesRead;
}

// Prüft auf Integer Overflow
int check_overflow(uint32_t a, uint32_t b) {
    return a > UINT32_MAX - b;
}

// Validiert die Checksumme (Dummy-Funktion, ersetzen Sie dies durch Ihren Algorithmus)
int validate_checksum(const struct file_header *header, const uint8_t *data) {
    // Hier eine einfache Beispielprüfung
    uint8_t dummy_checksum[32] = {0};
    return memcmp(header->checksum, dummy_checksum, 32) == 0;
}

// Funktion zum Parsen der Datei
void parse_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Fehler beim Öffnen der Datei");
        exit(EXIT_FAILURE);
    }

    struct file_header header;

    // Lese den Header sicher
    safe_read(&header, sizeof(struct file_header), 1, file);

    // Validierung des Magic-Wertes
    if (header.magic != EXPECTED_MAGIC) {
        fprintf(stderr, "Ungültiger Magic-Wert: 0x%x\n", header.magic);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Validierung der Version (Beispielgrenze)
    if (header.version > 5) {
        fprintf(stderr, "Nicht unterstützte Version: %u\n", header.version);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Prüfung auf Integer Overflow bei data_size
    if (header.data_size == 0 || check_overflow(header.data_size, sizeof(struct file_header))) {
        fprintf(stderr, "Ungültige oder zu große Datenmenge: %u\n", header.data_size);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Allokiere Speicher für die Daten
    uint8_t *data = malloc(header.data_size);
    if (!data) {
        perror("Speicherallokierung fehlgeschlagen");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Lese die Daten sicher
    safe_read(data, 1, header.data_size, file);

    // Validierung der Checksumme
    if (!validate_checksum(&header, data)) {
        fprintf(stderr, "Ungültige Checksumme\n");
        free(data);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Verarbeitung der Daten (hier könnten weitere Schritte erfolgen)
    printf("Datei erfolgreich geparst\n");

    // Ressourcen freigeben
    free(data);
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    parse_file(argv[1]);
    return EXIT_SUCCESS;
}
