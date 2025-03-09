#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024
#define MAX_KEY_LENGTH 128
#define MAX_VALUE_LENGTH 512
#define LOG_FILE "config_generator.log"
#define TEMPLATE_FILE "template.conf"
#define OUTPUT_FILE "output.conf"

// Struktur für Key-Value Paare
typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
} ConfigEntry;

// Logger Funktionen
void log_message(const char* level, const char* message) {
    FILE* log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Fehler beim Öffnen der Log-Datei: %s\n", strerror(errno));
        return;
    }

    time_t now;
    time(&now);
    char* date = ctime(&now);
    date[strlen(date) - 1] = '\0'; // Entferne newline

    fprintf(log_file, "[%s] [%s] %s\n", date, level, message);
    fclose(log_file);
}

// Input Validierung
int validate_key(const char* key) {
    if (strlen(key) >= MAX_KEY_LENGTH) {
        log_message("ERROR", "Schlüssel zu lang");
        return 0;
    }

    // Erlaube nur alphanumerische Zeichen und Unterstriche
    for (int i = 0; key[i]; i++) {
        if (!isalnum(key[i]) && key[i] != '_') {
            log_message("ERROR", "Ungültige Zeichen im Schlüssel");
            return 0;
        }
    }
    return 1;
}

int validate_value(const char* value) {
    if (strlen(value) >= MAX_VALUE_LENGTH) {
        log_message("ERROR", "Wert zu lang");
        return 0;
    }

    // Grundlegende Überprüfung auf schädliche Zeichen
    if (strchr(value, ';') || strchr(value, '|') || strchr(value, '`')) {
        log_message("ERROR", "Potenziell gefährliche Zeichen im Wert");
        return 0;
    }
    return 1;
}

// Template Verarbeitung
int process_template(ConfigEntry* entries, int* entry_count) {
    FILE* template_file = fopen(TEMPLATE_FILE, "r");
    if (template_file == NULL) {
        log_message("ERROR", "Template-Datei konnte nicht geöffnet werden");
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    *entry_count = 0;

    while (fgets(line, sizeof(line), template_file) != NULL) {
        // Ignoriere Kommentare und leere Zeilen
        if (line[0] == '#' || line[0] == '\n') continue;

        char key[MAX_KEY_LENGTH];
        char value[MAX_VALUE_LENGTH];

        // Parsen der Template-Zeile (Format: key=defaultvalue)
        if (sscanf(line, "%[^=]=%[^\n]", key, value) == 2) {
            // Entferne Whitespace
            char* k = key;
            while (isspace(*k)) k++;
            char* v = value;
            while (isspace(*v)) v++;

            if (validate_key(k) && validate_value(v)) {
                strncpy(entries[*entry_count].key, k, MAX_KEY_LENGTH - 1);
                strncpy(entries[*entry_count].value, v, MAX_VALUE_LENGTH - 1);
                (*entry_count)++;
            }
        }
    }

    fclose(template_file);
    return 1;
}

// Sichere Dateioperationen
int write_config(const ConfigEntry* entries, int entry_count) {
    // Erstelle temporäre Datei
    char temp_file[] = "temp_XXXXXX";
    int fd = mkstemp(temp_file);
    if (fd == -1) {
        log_message("ERROR", "Temporäre Datei konnte nicht erstellt werden");
        return 0;
    }

    FILE* temp = fdopen(fd, "w");
    if (temp == NULL) {
        log_message("ERROR", "Temporäre Datei konnte nicht geöffnet werden");
        close(fd);
        return 0;
    }

    // Schreibe Header
    fprintf(temp, "# Automatisch generierte Konfigurationsdatei\n");
    fprintf(temp, "# Erstellt: %s\n\n", ctime(&(time_t){time(NULL)}));

    // Schreibe Einträge
    for (int i = 0; i < entry_count; i++) {
        fprintf(temp, "%s=%s\n", entries[i].key, entries[i].value);
    }

    fclose(temp);

    // Atomares Umbenennen
    if (rename(temp_file, OUTPUT_FILE) != 0) {
        log_message("ERROR", "Konnte temporäre Datei nicht umbenennen");
        unlink(temp_file);
        return 0;
    }

    log_message("INFO", "Konfigurationsdatei erfolgreich erstellt");
    return 1;
}

// Hauptfunktion
int main() {
    ConfigEntry entries[100];  // Array für Config-Einträge
    int entry_count = 0;

    log_message("INFO", "Starte Config Generator");

    if (!process_template(entries, &entry_count)) {
        fprintf(stderr, "Fehler beim Verarbeiten des Templates\n");
        return 1;
    }

    // Hier könnte man interaktive Benutzereingaben für Werte hinzufügen
    
    if (!write_config(entries, entry_count)) {
        fprintf(stderr, "Fehler beim Schreiben der Konfigurationsdatei\n");
        return 1;
    }

    printf("Konfigurationsdatei erfolgreich erstellt.\n");
    return 0;
}