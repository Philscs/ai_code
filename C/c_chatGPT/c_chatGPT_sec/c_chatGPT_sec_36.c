#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define MAX_MESSAGE_SIZE 256
#define LOG_FILE "process_comm.log"
#define MAX_RESOURCES 10

// Struct für Nachrichten
typedef struct {
    char content[MAX_MESSAGE_SIZE];
} Message;

// Ressourcen-Tracking
int active_resources = 0;

// Prototypen
void log_event(const char *event);
int validate_message(const Message *msg);
Message *allocate_message(const char *content);
void free_message(Message *msg);
void handle_error(const char *error_msg);
void process_message(Message *msg);

// Logging-System
void log_event(const char *event) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Fehler beim Öffnen der Log-Datei: %s\n", strerror(errno));
        return;
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Neue Zeile entfernen

    fprintf(log_file, "%s: %s\n", timestamp, event);
    fclose(log_file);
}

// Nachrichtvalidierung
int validate_message(const Message *msg) {
    if (msg == NULL || strlen(msg->content) == 0) {
        log_event("Nachrichtvalidierung fehlgeschlagen: Ungültige Nachricht");
        return 0;
    }

    if (strlen(msg->content) >= MAX_MESSAGE_SIZE) {
        log_event("Nachrichtvalidierung fehlgeschlagen: Nachricht zu groß");
        return 0;
    }

    return 1;
}

// Speicherverwaltung
Message *allocate_message(const char *content) {
    if (active_resources >= MAX_RESOURCES) {
        log_event("Ressourcenlimit erreicht. Nachricht kann nicht zugewiesen werden.");
        return NULL;
    }

    Message *msg = (Message *)malloc(sizeof(Message));
    if (msg == NULL) {
        handle_error("Speicherzuweisung fehlgeschlagen");
        return NULL;
    }

    strncpy(msg->content, content, MAX_MESSAGE_SIZE - 1);
    msg->content[MAX_MESSAGE_SIZE - 1] = '\0';

    active_resources++;
    log_event("Nachricht erfolgreich zugewiesen");
    return msg;
}

void free_message(Message *msg) {
    if (msg == NULL) return;

    free(msg);
    active_resources--;
    log_event("Nachricht erfolgreich freigegeben");
}

// Fehlerbehandlung
void handle_error(const char *error_msg) {
    log_event(error_msg);
    fprintf(stderr, "Fehler: %s\n", error_msg);
    exit(EXIT_FAILURE);
}

// Nachrichtenverarbeitung
void process_message(Message *msg) {
    if (!validate_message(msg)) {
        handle_error("Ungültige Nachricht");
    }

    printf("Nachricht verarbeitet: %s\n", msg->content);
    log_event("Nachricht erfolgreich verarbeitet");
}

// Beispielmain-Funktion
int main() {
    log_event("Programm gestartet");

    Message *msg = allocate_message("Hallo, Prozess!");
    if (msg == NULL) {
        handle_error("Nachricht konnte nicht erstellt werden");
    }

    process_message(msg);

    free_message(msg);

    log_event("Programm beendet");
    return 0;
}
