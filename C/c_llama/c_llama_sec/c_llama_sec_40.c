#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Typen für die verschiedenen Arten von Log-Einrichtungen
typedef enum {
    PROTOCOL_LOG,
    ERROR_LOG,
    WARNING_LOG
} LOG_TYPE;

// Struct für die Datei-Managements
typedef struct {
    char* filename;
    size_t max_size;
} FILEMANAGEMENT;

// Prototyp für die Funktionen
void log_message(LOG_TYPE type, const char* message);
void create_filemanager(FILEMANAGEMENT* fileman, LOG_TYPE type);
void close_filemanager(FILEMANAGEMENT* fileman);
int rotate_logs(FILEMANAGEMENT* fileman);

// Atombeschränkung für thread-sichere Zugriffe auf die Dateien
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Funktion zur Log-Meldung
void log_message(LOG_TYPE type, const char* message) {
    // Überprüfe ob der Typ gültig ist
    if (type == PROTOCOL_LOG || type == ERROR_LOG || type == WARNING_LOG) {
        // Erstelle eine Datei-Managements-Instanz
        FILEMANAGEMENT fileman;
        create_filemanager(&fileman, type);

        // Schreibe die Meldung in die entsprechende Datei
        switch (type) {
            case PROTOCOL_LOG:
                fprintf(fileman.filename, "%s\n", message);
                break;
            case ERROR_LOG:
                fprintf(fileman.filename, "ERROR: %s\n", message);
                break;
            case WARNING_LOG:
                fprintf(fileman.filename, "WARNING: %s\n", message);
                break;
        }

        // Schließe die Datei-Managements-Instanz
        close_filemanager(&fileman);

    } else {
        printf("Gültiger Log-Typ nicht gefunden.\n");
    }
}

// Funktion zum Erstellen einer Datei-Managements-Instanz
void create_filemanager(FILEMANAGEMENT* fileman, LOG_TYPE type) {
    // Erstelle die Dateinamen basierend auf dem Typ
    char filename[256];
    switch (type) {
        case PROTOCOL_LOG:
            sprintf(filename, "protocol_log_%d.log", getpid());
            break;
        case ERROR_LOG:
            sprintf(filename, "error_log_%d.log", getpid());
            break;
        case WARNING_LOG:
            sprintf(filename, "warning_log_%d.log", getpid());
            break;
    }

    // Erstelle die Datei-Managements-Instanz
    fileman->filename = filename;
    fileman->max_size = 1024 * 1024; // 1 MB

    // Erstelle die Dateien
    FILE* fp = fopen(fileman->filename, "a");
    if (fp == NULL) {
        printf("Konnte Datei '%s' nicht eröffnen.\n", fileman->filename);
        exit(1);
    }

    fclose(fp);

}

// Funktion zum Schließen einer Datei-Managements-Instanz
void close_filemanager(FILEMANAGEMENT* fileman) {
    // Überprüfe ob die Datei-Managements-Instanz bereits geschlossen ist
    if (fclose(fileman->filename, "w") != 0) {
        printf("Konnte Datei '%s' nicht schließen.\n", fileman->filename);
        exit(1);
    }

}

// Funktion zum Spülen der Log-Einträge aus einer Datei
int rotate_logs(FILEMANAGEMENT* fileman) {
    // Überprüfe ob die Datei bereits voll ist
    FILE* fp = fopen(fileman->filename, "r");
    if (fp == NULL) return 0;
    fseek(fp, 0, SEEK_END);
    off_t size = ftell(fp);
    rewind(fp);

    // Wenn die Datei zu groß ist, entferne sie
    if (size > fileman->max_size / 2) {
        printf("Log-Datei '%s' ist zu vollständig. Entferne... ", fileman->filename);
        unlink(fileman->filename);
    }

    fclose(fp);

}

int main() {
    // Erstelle eine Log-Meldung
    log_message(PROTOCOL_LOG, "Protokoll-Eintrag");

    // Warte für 5 Sekunden und dann löse den Thread
    sleep(5);
    pthread_mutex_lock(&mutex);
    rotate_logs((FILEMANAGEMENT*)malloc(sizeof(FILEMANAGEMENT)));
    pthread_mutex_unlock(&mutex);

    return 0;
}
