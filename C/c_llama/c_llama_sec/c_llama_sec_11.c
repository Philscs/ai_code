#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

// Maximal Länge für eine Argument-Eingabe
#define MAX_ARG_LENGTH 1024

// Sicherheitsparameter
#define MAX_SAFE_ARGS 10 // Anzahl an sicheren Argumenten
#define SAFE_ARGS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

// Command Line Parser-Klasse
typedef struct {
    char *argv[MAX_SAFE_ARGS + 1]; // Argumente
    int argc; // Argumentanzahl
} CommandLineParser;

// Konstruktor für den Parser
CommandLineParser* create_parser() {
    CommandLineParser* parser = malloc(sizeof(CommandLineParser));
    if (!parser) {
        return NULL;
    }
    parser->argv[0] = NULL; // Null-Argument (Optional)
    parser->argc = 0;
    return parser;
}

// Verarbeitet ein Argument und überprüft seine Gültigkeit
bool validate_arg(const char* arg) {
    int len = strlen(arg);
    if (len > MAX_ARG_LENGTH || strchr(SAFE_ARGS, arg[0])) {
        fprintf(stderr, "Fehler: Argument '%s' ist zu lang oder enthält nicht gültige Zeichen.\n", arg);
        return false;
    }
    for (int i = 1; i < len; ++i) {
        if (!strchr(SAFE_ARGS, arg[i])) {
            fprintf(stderr, "Fehler: Argument '%s' ist zu lang oder enthält nicht gültige Zeichen.\n", arg);
            return false;
        }
    }
    return true;
}

// Konvertiert ein String in eine Ganzzahl
int str_to_int(const char* str) {
    int val = 0;
    bool neg = false;
    if (str[0] == '-') {
        neg = true;
        str++;
    }
    for (; *str != '\0'; str++) {
        val *= 10;
        val += (*str - '0');
    }
    return neg ? -val : val;
}

// Loggt eine Eingabe für Audit-Zwecke
void log_input(const char* arg) {
    fprintf(stderr, "[Audit] Argument '%s' eingereicht.\n", arg);
}

// Verarbeitet die Eingaben und gibt die Ergebnisse zurück
int parse_args(CommandLineParser* parser, int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (!validate_arg(argv[i])) {
            return -1;
        }
        log_input(argv[i]);
        // Verarbeiten des Arguments (z.B. Konvertieren in eine Ganzzahl)
        parser->argv[parser->argc++] = malloc(strlen(argv[i]) + 1);
        strcpy(parser->argv[parser->argc], argv[i]);

    }
    return 0;
}

// Freigibt den Parser
void free_parser(CommandLineParser* parser) {
    for (int i = 0; i < parser->argc; ++i) {
        free(parser->argv[i]);
    }
    free(parser);
}

int main(int argc, char** argv) {
    CommandLineParser* parser = create_parser();

    int ret = parse_args(parser, argc, argv);

    if (ret != -1) {
        for (int i = 0; i < parser->argc; ++i) {
            printf("%s\n", parser->argv[i]);
        }
    }

    free_parser(parser);
    return ret;
}