#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>

#define MAX_ARG_LENGTH 1024
#define MAX_LOG_LENGTH 2048
#define LOG_FILE "cmd_audit.log"

// Struktur für geparste Argumente
typedef struct {
    char original_arg[MAX_ARG_LENGTH];
    long number_value;
    bool is_number;
    bool is_valid;
} ParsedArg;

// Funktion zum sicheren Logging
void log_command(const char* program_name, int argc, char** argv) {
    time_t now;
    time(&now);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';  // Entferne newline

    FILE* log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Fehler beim Öffnen der Log-Datei\n");
        return;
    }

    char log_buffer[MAX_LOG_LENGTH] = {0};
    int offset = snprintf(log_buffer, MAX_LOG_LENGTH, "[%s] %s", timestamp, program_name);
    
    for (int i = 1; i < argc && offset < MAX_LOG_LENGTH; i++) {
        offset += snprintf(log_buffer + offset, MAX_LOG_LENGTH - offset, " %s", argv[i]);
    }

    fprintf(log_file, "%s\n", log_buffer);
    fclose(log_file);
}

// Funktion zur Überprüfung auf potenziell gefährliche Zeichen
bool contains_dangerous_chars(const char* arg) {
    const char* dangerous_chars = ";&|`$()<>\\\"'";
    return (strpbrk(arg, dangerous_chars) != NULL);
}

// Sichere Konvertierung von String zu Long
bool safe_strtol(const char* str, long* result) {
    if (str == NULL || result == NULL) {
        return false;
    }

    char* endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if (errno == ERANGE) {
        return false;
    }

    if (endptr == str || *endptr != '\0') {
        return false;
    }

    *result = val;
    return true;
}

// Hauptfunktion zum Parsen der Argumente
ParsedArg* parse_arguments(int argc, char** argv, int* parsed_count) {
    if (argc < 1 || argv == NULL || parsed_count == NULL) {
        return NULL;
    }

    // Logging der Kommandozeilen-Eingabe
    log_command(argv[0], argc, argv);

    // Allokiere Speicher für geparste Argumente
    ParsedArg* parsed_args = calloc(argc - 1, sizeof(ParsedArg));
    if (parsed_args == NULL) {
        fprintf(stderr, "Speicherallokierung fehlgeschlagen\n");
        return NULL;
    }

    *parsed_count = argc - 1;

    // Parse jedes Argument
    for (int i = 1; i < argc; i++) {
        ParsedArg* current = &parsed_args[i-1];
        
        // Überprüfe Argumentlänge
        if (strlen(argv[i]) >= MAX_ARG_LENGTH) {
            fprintf(stderr, "Argument %d ist zu lang (max %d Zeichen)\n", 
                    i, MAX_ARG_LENGTH-1);
            current->is_valid = false;
            continue;
        }

        // Kopiere Original-Argument
        strncpy(current->original_arg, argv[i], MAX_ARG_LENGTH-1);
        current->original_arg[MAX_ARG_LENGTH-1] = '\0';

        // Überprüfe auf gefährliche Zeichen
        if (contains_dangerous_chars(current->original_arg)) {
            fprintf(stderr, "Argument %d enthält unerlaubte Zeichen\n", i);
            current->is_valid = false;
            continue;
        }

        // Versuche Konvertierung zu Zahl
        current->is_number = safe_strtol(current->original_arg, &current->number_value);
        current->is_valid = true;
    }

    return parsed_args;
}

// Funktion zum Freigeben des Speichers
void free_parsed_args(ParsedArg* args) {
    free(args);
}

// Beispiel zur Verwendung
int main(int argc, char** argv) {
    int parsed_count;
    ParsedArg* parsed_args = parse_arguments(argc, argv, &parsed_count);

    if (parsed_args == NULL) {
        fprintf(stderr, "Fehler beim Parsen der Argumente\n");
        return 1;
    }

    // Verarbeite die geparsten Argumente
    printf("Geparste Argumente:\n");
    for (int i = 0; i < parsed_count; i++) {
        printf("Argument %d:\n", i+1);
        printf("  Original: %s\n", parsed_args[i].original_arg);
        printf("  Gültig: %s\n", parsed_args[i].is_valid ? "ja" : "nein");
        if (parsed_args[i].is_number) {
            printf("  Numerischer Wert: %ld\n", parsed_args[i].number_value);
        }
        printf("\n");
    }

    free_parsed_args(parsed_args);
    return 0;
}