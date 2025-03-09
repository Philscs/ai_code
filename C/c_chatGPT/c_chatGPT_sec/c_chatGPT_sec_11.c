#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_ARG_LENGTH 100

// Logging Funktion
void log_input(const char *input) {
    FILE *logfile = fopen("cli_parser.log", "a");
    if (logfile) {
        fprintf(logfile, "%s\n", input);
        fclose(logfile);
    } else {
        fprintf(stderr, "Fehler beim Öffnen der Log-Datei!\n");
    }
}

// Validierung von numerischen Eingaben
bool validate_numeric_input(const char *input) {
    for (size_t i = 0; i < strlen(input); i++) {
        if (!isdigit(input[i]) && input[i] != '-' && input[i] != '.') {
            return false;
        }
    }
    return true;
}

// Sichere Konvertierung von String zu Integer
bool safe_str_to_int(const char *str, int *result) {
    errno = 0;
    char *endptr;
    long val = strtol(str, &endptr, 10);

    if (errno != 0 || *endptr != '\0' || val > INT_MAX || val < INT_MIN) {
        return false;
    }
    *result = (int)val;
    return true;
}

// Sichere Konvertierung von String zu Double
bool safe_str_to_double(const char *str, double *result) {
    errno = 0;
    char *endptr;
    double val = strtod(str, &endptr);

    if (errno != 0 || *endptr != '\0') {
        return false;
    }
    *result = val;
    return true;
}

// Sicherer Argument-Parser
void parse_arguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        // Logge jedes Argument
        log_input(argv[i]);

        // Verhindere zu lange Argumente
        if (strlen(argv[i]) > MAX_ARG_LENGTH) {
            fprintf(stderr, "Fehler: Argument zu lang (%s).\n", argv[i]);
            exit(EXIT_FAILURE);
        }

        // Validierung gegen Command Injection (nur alphanumerische Zeichen erlauben)
        for (size_t j = 0; j < strlen(argv[i]); j++) {
            if (!isalnum(argv[i][j]) && argv[i][j] != '-' && argv[i][j] != '.') {
                fprintf(stderr, "Fehler: Ungültiges Zeichen im Argument (%s).\n", argv[i]);
                exit(EXIT_FAILURE);
            }
        }

        // Beispiel: Verarbeitung von numerischen Werten
        int int_value;
        double double_value;

        if (validate_numeric_input(argv[i])) {
            if (safe_str_to_int(argv[i], &int_value)) {
                printf("Gültige Integer-Eingabe: %d\n", int_value);
            } else if (safe_str_to_double(argv[i], &double_value)) {
                printf("Gültige Double-Eingabe: %.2f\n", double_value);
            } else {
                fprintf(stderr, "Fehler: Ungültige numerische Eingabe (%s).\n", argv[i]);
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Nicht-numerisches Argument: %s\n", argv[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Verwendung: %s <argumente>\n", argv[0]);
        return EXIT_FAILURE;
    }

    parse_arguments(argc, argv);
    return EXIT_SUCCESS;
}
