#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define DNS_MAX_MESSAGE_SIZE 512
#define LOG_FILE "dns_parser.log"

// DNS Header Structure
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
} DNSHeader;

// Logging function
void log_message(const char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

// Validate DNS protocol basics
bool validate_dns_header(const DNSHeader *header) {
    if (ntohs(header->qd_count) == 0) {
        log_message("Error: No questions in DNS query.");
        return false;
    }
    return true;
}

// Parse a DNS Query
bool parse_dns_query(const uint8_t *buffer, size_t size) {
    if (size < sizeof(DNSHeader)) {
        log_message("Error: Buffer size too small for DNS header.");
        return false;
    }

    // Parse DNS Header
    const DNSHeader *header = (const DNSHeader *)buffer;

    if (!validate_dns_header(header)) {
        return false;
    }

    size_t offset = sizeof(DNSHeader);

    // Parse Questions
    for (int i = 0; i < ntohs(header->qd_count); ++i) {
        // Parse QNAME (Variable-length domain name)
        while (offset < size && buffer[offset] != 0) {
            uint8_t label_length = buffer[offset];
            if (label_length > 63 || offset + label_length + 1 > size) {
                log_message("Error: Invalid domain name format.");
                return false;
            }
            offset += label_length + 1;
        }

        if (offset >= size) {
            log_message("Error: QNAME exceeds buffer size.");
            return false;
        }
        offset++; // Skip null byte

        // Parse QTYPE and QCLASS
        if (offset + 4 > size) {
            log_message("Error: Buffer size too small for QTYPE and QCLASS.");
            return false;
        }
        offset += 4;
    }

    log_message("DNS query parsed successfully.");
    return true;
}

int main() {
    // Example usage
    uint8_t buffer[DNS_MAX_MESSAGE_SIZE];
    size_t received_size;

    // Simulate receiving a DNS query (In real scenarios, use recvfrom() or similar)
    FILE *sample_query = fopen("sample_dns_query.bin", "rb");
    if (!sample_query) {
        perror("Error opening sample DNS query file");
        return EXIT_FAILURE;
    }

    received_size = fread(buffer, 1, DNS_MAX_MESSAGE_SIZE, sample_query);
    fclose(sample_query);

    if (received_size == 0) {
        log_message("Error: No data read from sample DNS query file.");
        return EXIT_FAILURE;
    }

    if (!parse_dns_query(buffer, received_size)) {
        log_message("Error: Failed to parse DNS query.");
        return EXIT_FAILURE;
    }

    log_message("DNS query processing completed successfully.");
    return EXIT_SUCCESS;
}