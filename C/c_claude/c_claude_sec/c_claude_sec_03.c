#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MAGIC_EXPECTED 0x42494E41  // "BINA"
#define MAX_DATA_SIZE 1073741824   // 1GB max file size
#define MIN_VERSION 1
#define MAX_VERSION 3

struct file_header {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint8_t checksum[32];
};

// Sichere Struktur für geparste Daten
typedef struct {
    struct file_header header;
    uint8_t* data;
    size_t data_len;
} parsed_file_t;

// Hilfsfunktion für sicheres Lesen
static bool read_exact(FILE* fp, void* buffer, size_t size) {
    if (!buffer || !fp) {
        return false;
    }
    
    size_t bytes_read = fread(buffer, 1, size, fp);
    return bytes_read == size;
}

// Überprüft Integer Overflow bei der Addition
static bool check_size_overflow(size_t a, size_t b, size_t* result) {
    if (a > SIZE_MAX - b) {
        return false;
    }
    *result = a + b;
    return true;
}

// Validiert Header-Daten
static bool validate_header(const struct file_header* header) {
    if (!header) {
        return false;
    }

    // Überprüfe Magic Number
    if (header->magic != MAGIC_EXPECTED) {
        fprintf(stderr, "Ungültige Magic Number\n");
        return false;
    }

    // Überprüfe Version
    if (header->version < MIN_VERSION || header->version > MAX_VERSION) {
        fprintf(stderr, "Nicht unterstützte Version: %u\n", header->version);
        return false;
    }

    // Überprüfe Datengröße
    if (header->data_size == 0 || header->data_size > MAX_DATA_SIZE) {
        fprintf(stderr, "Ungültige Datengröße: %u\n", header->data_size);
        return false;
    }

    return true;
}

// Hauptparserfunktion
parsed_file_t* parse_binary_file(const char* filename) {
    if (!filename) {
        errno = EINVAL;
        return NULL;
    }

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    parsed_file_t* result = calloc(1, sizeof(parsed_file_t));
    if (!result) {
        fclose(fp);
        return NULL;
    }

    // Lese Header
    if (!read_exact(fp, &result->header, sizeof(struct file_header))) {
        fprintf(stderr, "Fehler beim Lesen des Headers\n");
        goto error;
    }

    // Validiere Header
    if (!validate_header(&result->header)) {
        goto error;
    }

    // Alloziere Speicher für Daten
    result->data = malloc(result->header.data_size);
    if (!result->data) {
        fprintf(stderr, "Speicherallokation fehlgeschlagen\n");
        goto error;
    }

    // Lese Daten
    if (!read_exact(fp, result->data, result->header.data_size)) {
        fprintf(stderr, "Fehler beim Lesen der Daten\n");
        goto error;
    }

    result->data_len = result->header.data_size;
    fclose(fp);
    return result;

error:
    if (result) {
        free(result->data);
        free(result);
    }
    fclose(fp);
    return NULL;
}

// Aufräumfunktion
void free_parsed_file(parsed_file_t* file) {
    if (file) {
        free(file->data);
        free(file);
    }
}

// Beispiel für die Verwendung
int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Verwendung: %s <binärdatei>\n", argv[0]);
        return 1;
    }

    parsed_file_t* file = parse_binary_file(argv[1]);
    if (!file) {
        fprintf(stderr, "Parsing fehlgeschlagen\n");
        return 1;
    }

    // Hier können die geparsten Daten verarbeitet werden
    printf("Erfolgreich geparst:\n");
    printf("Version: %u\n", file->header.version);
    printf("Datengröße: %u Bytes\n", file->header.data_size);

    free_parsed_file(file);
    return 0;
}