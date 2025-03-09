#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Prototypen für die Funktionen
void parse_buffer(char *buffer, size_t bufferSize);
bool validate_protocol(const char *protocol);
void log_message(const char *message);
bool handle_error(const char *error);

// State-Machine-Variable für den aktuellen Zustand
enum state { INIT, HEADER, BODY } currentState = INIT;

// Buffer-Variable zur Speicherung des aktuell gelesenen Buffers
char currentBuffer[1024];

// Überprüfung auf das korrekte Protokoll
bool validate_protocol(const char *protocol) {
    if (strcmp(protocol, "HTTP/1.1") == 0 || strcmp(protocol, "HTTP/2.0") == 0) {
        return true;
    } else {
        printf("Falsches Protokoll! (%s)\n", protocol);
        return false;
    }
}

// Funktion für die Log-Meldung
void log_message(const char *message) {
    printf("%s\n", message);
}

// Funktion für das Handhaben von Fehlern
bool handle_error(const char *error) {
    if (strcmp(error, "HTTP/1.0") == 0 || strcmp(error, "HTTP/2.1") == 0) {
        printf("Falsches Protokoll! (%s)\n", error);
        return true;
    } else {
        printf("Unbekannte Fehlermeldung! (%s)\n", error);
        return false;
    }
}

// Funktion für das Lesen eines Bytes aus dem Buffer
char read_byte(char *buffer, size_t bufferSize) {
    if (*buffer == '\r' && *(buffer+1) == '\n') {
        return '\0';
    } else {
        return *buffer++;
    }
}

// Funktion zum Parsen des Protokolls
void parse_buffer(char *buffer, size_t bufferSize) {
    while (true) {
        // Lesen eines Bytes aus dem Buffer
        char byte = read_byte(buffer, bufferSize);

        if (byte == '\0') {
            break;
        }

        switch (currentState) {
            case INIT:
                if (is_header_byte(byte)) {
                    currentState = HEADER;
                } else {
                    currentState = BODY;
                }
                break;

            case HEADER:
                if (is_end_of_header_byte(byte)) {
                    currentState = BODY;
                } else {
                    printf("Falsches Protokoll! (%c)\n", byte);
                    return;
                }

            case BODY:
                // Parsen des Körpers
                for (size_t i = 0; i < bufferSize; i++) {
                    if (buffer[i] == '\r' && buffer[i+1] == '\n') {
                        printf("Ende des Protokolls!\n");
                        return;
                    }
                }
        }
    }

}

// Funktionen für die Überprüfung auf das korrekte Protokoll
bool is_header_byte(char byte) {
    return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z');
}

bool is_end_of_header_byte(char byte) {
    return (byte >= '0' && byte <= '9');
}

int main() {
    // Beispiel-Buffer
    char buffer[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nHello World!";
    
    // Buffer parsen
    parse_buffer(buffer, sizeof(buffer));

    return 0;
}