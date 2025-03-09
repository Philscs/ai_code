#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DNS-Packet-Struktur
typedef struct {
    char qdcount;
    unsigned short id;
    unsigned short qr;
    unsigned short flags;
    unsigned short trunc;  // Truncation Flag
    unsigned int dlen;
    unsigned char tsval[8];  // Time Stamp
    unsigned char rdflags[2];
} dns_header;

// DNS-Questions-Struktur
typedef struct {
    char qtype;
    char qclass;
    char qname[256];
} dns_question;

// DNS-Record-Struktur
typedef struct {
    unsigned short type;
    unsigned short class;
    char ptr[1024];  // Pointer zur Antwort
} dns_record;

int main() {
    // Konfiguration
    const int MAX_BUFFER_SIZE = 4096;  // Buffer-Größe in Bytes

    // Initialisierung der Log-Eingabe
    FILE *log_file = fopen("dns_parser.log", "w");
    if (!log_file) {
        printf("Fehler: Konnten nicht auf Log-Datei zugreifen.\n");
        return 1;
    }

    // DNS-Packet-Empfang und Verarbeitung
    char buffer[MAX_BUFFER_SIZE];
    int packet_size = recv(STDIN_FILENO, buffer, MAX_BUFFER_SIZE, 0);
    if (packet_size <= 0) {
        printf("Fehler: Keine Daten empfangen.\n");
        return 1;
    }

    // Protokollvalidierung
    dns_header *header = (dns_header *)buffer;
    if (*header == 0 || header->qdcount > 1 || header->id <= 0 || header->qr != 0) {
        printf("Fehler: Ungültige DNS-Paketstruktur.\n");
        fclose(log_file);
        return 1;
    }

    // Buffer-Überlastungsbewältigung
    unsigned int total_length = *header->dlen;

    // Fehlerbehandlung und Log-Eingabe
    for (int i = 0; i < packet_size; i++) {
        char c = buffer[i];
        if (i == 0 && !c) {  // Nullzeichen anfangen?
            printf("Fehler: DNS-Paket-Ende vorzeitig.\n");
            fclose(log_file);
            return 1;
        } else {
            char tmp[10];  // Buffer für temporary Daten
            sprintf(tmp, "Datenpunkte %d (%c):", i, c);  // Log-Eingabe von Datenpunkt
            fwrite(tmp, strlen(tmp), 1, log_file);

            if (i < total_length) {  // Überprüfung der Antwortgröße
                char *ptr = buffer + i;
                dns_record *record = (dns_record *)ptr;
                for (int j = 0; j < record->type; j++) {
                    char tmp2[1024];  // Buffer für temporary Daten
                    sprintf(tmp2, "Typ %d (%c):", record->type, c);  // Log-Eingabe von Typ
                    fwrite(tmp2, strlen(tmp2), 1, log_file);

                    if (j == record->ptr - 1) {  // Überprüfung des letzten Punkt-Tests
                        printf("Fehler: Ungültige DNS-Rekordstruktur.\n");
                        fclose(log_file);
                        return 1;
                    }
                }
            } else {
                printf("Fehler: DNS-Paket-Ende vorzeitig.\n");
                fclose(log_file);
                return 1;
            }
        }
    }

    fclose(log_file);
    return 0;

}
