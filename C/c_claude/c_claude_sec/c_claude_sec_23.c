#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

#define MAX_DNS_SIZE 512        // Maximum size eines DNS-Pakets
#define MAX_LABEL_LENGTH 63     // Maximum Länge eines DNS-Labels
#define MAX_NAME_LENGTH 255     // Maximum Länge eines DNS-Namens
#define LOG_BUFFER_SIZE 1024    // Größe des Log-Puffers

// Struktur für DNS-Header
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} DNSHeader;

// Logging-Level
typedef enum {
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
} LogLevel;

// Globale Logging-Funktion
void log_message(LogLevel level, const char* format, ...) {
    time_t now;
    time(&now);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';  // Entferne newline

    va_list args;
    va_start(args, format);
    
    char log_buffer[LOG_BUFFER_SIZE];
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);

    const char* level_str;
    switch (level) {
        case LOG_ERROR:   level_str = "ERROR"; break;
        case LOG_WARNING: level_str = "WARNING"; break;
        case LOG_INFO:    level_str = "INFO"; break;
        case LOG_DEBUG:   level_str = "DEBUG"; break;
        default:          level_str = "UNKNOWN";
    }

    fprintf(stderr, "[%s] %s: %s\n", timestamp, level_str, log_buffer);
    va_end(args);
}

// Sicheres Memory Management
void* secure_malloc(size_t size) {
    if (size == 0 || size > MAX_DNS_SIZE) {
        log_message(LOG_ERROR, "Ungültige Speicheranforderung: %zu Bytes", size);
        return NULL;
    }

    void* ptr = calloc(1, size);  // Verwende calloc für initiale Nullsetzung
    if (ptr == NULL) {
        log_message(LOG_ERROR, "Speicherallokation fehlgeschlagen: %s", strerror(errno));
        return NULL;
    }

    return ptr;
}

// Sicheres Lesen von DNS-Labels
bool read_dns_label(const uint8_t* buffer, size_t buffer_size, size_t* offset, char* name, size_t name_size) {
    if (buffer == NULL || name == NULL || offset == NULL) {
        log_message(LOG_ERROR, "Ungültige Parameter beim Label-Lesen");
        return false;
    }

    size_t original_offset = *offset;
    size_t name_offset = 0;
    size_t jumps = 0;
    const size_t max_jumps = 10;  // Verhindern von endlosen Compression-Loops

    while (*offset < buffer_size) {
        uint8_t length = buffer[*offset];

        // Überprüfe Compression-Pointer
        if ((length & 0xC0) == 0xC0) {
            if (*offset + 1 >= buffer_size) {
                log_message(LOG_ERROR, "Ungültiger Compression-Pointer");
                return false;
            }

            if (++jumps > max_jumps) {
                log_message(LOG_ERROR, "Zu viele Compression-Sprünge");
                return false;
            }

            uint16_t pointer = ((length & 0x3F) << 8) | buffer[*offset + 1];
            if (pointer >= original_offset) {
                log_message(LOG_ERROR, "Vorwärts-Pointer nicht erlaubt");
                return false;
            }

            *offset = pointer;
            continue;
        }

        // Normales Label
        if (length == 0) {
            // Ende des Namens
            if (name_offset > 0 && name_offset < name_size) {
                name[name_offset - 1] = '\0';  // Ersetze letzten Punkt
            }
            (*offset)++;
            return true;
        }

        if (length > MAX_LABEL_LENGTH) {
            log_message(LOG_ERROR, "Label-Länge überschreitet Maximum: %d", length);
            return false;
        }

        if (*offset + length + 1 > buffer_size) {
            log_message(LOG_ERROR, "Label überschreitet Puffer-Grenzen");
            return false;
        }

        // Überprüfe verfügbaren Platz im Namen-Puffer
        if (name_offset + length + 1 >= name_size) {
            log_message(LOG_ERROR, "Name-Puffer zu klein");
            return false;
        }

        // Kopiere Label-Zeichen
        memcpy(name + name_offset, buffer + *offset + 1, length);
        name_offset += length;
        name[name_offset++] = '.';
        *offset += length + 1;
    }

    log_message(LOG_ERROR, "Unerwartetes Ende des Puffers");
    return false;
}

// Hauptfunktion zum Parsen eines DNS-Queries
bool parse_dns_query(const uint8_t* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < sizeof(DNSHeader)) {
        log_message(LOG_ERROR, "Ungültiger DNS-Query Buffer");
        return false;
    }

    if (buffer_size > MAX_DNS_SIZE) {
        log_message(LOG_ERROR, "DNS-Query zu groß: %zu Bytes", buffer_size);
        return false;
    }

    // Parse Header
    DNSHeader header;
    memcpy(&header, buffer, sizeof(DNSHeader));

    // Network to Host byte order
    header.id = ntohs(header.id);
    header.flags = ntohs(header.flags);
    header.qdcount = ntohs(header.qdcount);
    header.ancount = ntohs(header.ancount);
    header.nscount = ntohs(header.nscount);
    header.arcount = ntohs(header.arcount);

    log_message(LOG_INFO, "Parsing DNS Query ID: %u", header.id);

    // Validiere Question Count
    if (header.qdcount == 0) {
        log_message(LOG_ERROR, "Keine Questions im Query");
        return false;
    }

    size_t offset = sizeof(DNSHeader);
    char name[MAX_NAME_LENGTH + 1];

    // Parse jede Question
    for (uint16_t i = 0; i < header.qdcount; i++) {
        if (!read_dns_label(buffer, buffer_size, &offset, name, sizeof(name))) {
            return false;
        }

        // Überprüfe Platz für TYPE und CLASS
        if (offset + 4 > buffer_size) {
            log_message(LOG_ERROR, "Unvollständige Question-Sektion");
            return false;
        }

        uint16_t qtype = ntohs(*(uint16_t*)(buffer + offset));
        uint16_t qclass = ntohs(*(uint16_t*)(buffer + offset + 2));
        
        log_message(LOG_INFO, "Question %d: Name=%s, Type=%u, Class=%u", 
                   i + 1, name, qtype, qclass);

        offset += 4;
    }

    return true;
}

// Beispiel für die Verwendung
int main(void) {
    // Beispiel DNS-Query (nur zu Demonstrationszwecken)
    uint8_t example_query[] = {
        0x12, 0x34,  // ID
        0x01, 0x00,  // Flags
        0x00, 0x01,  // QDCOUNT
        0x00, 0x00,  // ANCOUNT
        0x00, 0x00,  // NSCOUNT
        0x00, 0x00,  // ARCOUNT
        // Question
        0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
        0x03, 'c', 'o', 'm',
        0x00,        // Name terminator
        0x00, 0x01,  // QTYPE (A)
        0x00, 0x01   // QCLASS (IN)
    };

    if (parse_dns_query(example_query, sizeof(example_query))) {
        log_message(LOG_INFO, "DNS-Query erfolgreich geparst");
    } else {
        log_message(LOG_ERROR, "Fehler beim Parsen des DNS-Query");
    }

    return 0;
}