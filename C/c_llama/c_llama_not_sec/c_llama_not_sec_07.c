#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Typen für den Parser
typedef enum {
    START,
    EPSILON,
    SCHARACTER,
    OR,
    STAR
} ParserType;

// Typen für die NFAs
typedef struct {
    ParserType type;
    int next_char_index;
    char* next_char;
} NFAState;

typedef struct {
    NFAState* start_state;
    NFAState** states;
    int num_states;
} NFA;

// Funktion zur Erstellung eines neuen NFAs
NFA* nfa_new() {
    NFA* nfa = malloc(sizeof(NFA));
    nfa->start_state = NULL;
    nfa->states = NULL;
    return nfa;
}

// Funktion, um ein neues State zu erstellen
void nfa_add_state(NFA* nfa, ParserType type, int next_char_index, char* next_char) {
    NFAState** new_states = realloc(nfa->states, (nfa->num_states + 1) * sizeof(NFAState*));
    if (new_states == NULL) {
        return;
    }
    new_states[nfa->num_states] = malloc(sizeof(NFAState));
    new_states[nfa->num_states]->type = type;
    new_states[nfa->num_states]->next_char_index = next_char_index;
    new_states[nfa->num_states]->next_char = strdup(next_char);
    nfa->states = new_states;
    nfa->num_states++;
}

// Funktion, um den Parser zu initialisieren
void nfa_init(NFA* nfa) {
    // Initialstate mit dem regulären Ausdruck '\u0000'
    nfa_add_state(nfa, EPSILON, 0, "\u0000");
}

// Funktion zur Konstruktions des NFAs
void nfa_construct(NFA* nfa, char* regex) {
    int i = 0;
    while (i < strlen(regex)) {
        // Faktor für die Reguläre Ausdrücke: 'a' -> '\u0001', '+' -> '\u0002', '?' -> '\u0003'
        switch (regex[i]) {
            case 'a':
                nfa_add_state(nfa, SCHARACTER, i + 1, "\u0001");
                break;
            case '+':
                nfa_add_state(nfa, STAR, -1, "\u0002");
                break;
            case '?':
                nfa_add_state(nfa, OR, i + 1, "\u0003");
                break;
            default:
                printf("Ungültige Reguläre Ausdrucksschreibweise\n");
                exit(1);
        }
        i++;
    }
}

// Funktion zur Durchführung der Parsenoperation
void nfa_parse(NFA* nfa, char input[]) {
    int current_state_index = 0;
    for (int i = 0; i < strlen(input); i++) {
        NFAState* current_state = nfa->states[current_state_index];
        if (strcmp(current_state->next_char, &input[i]) == 0) {
            // Wenn der aktuellen State die Eingabe passt
            current_state_index = current_state->next_char_index;
        } else {
            // Wenn der aktuellen State die Eingabe nicht passt und es keinen ε-Transition gibt
            printf("Ungültige Eingabe\n");
            return;
        }
    }

    // Durchführen von ε-Tunneln
    for (int i = 0; i < nfa->num_states; i++) {
        if (strcmp(nfa->states[i]->next_char, "\u0000") == 0) {
            current_state_index = i;
            break;
        }
    }

    // Eingabe ist erfolgreich
    printf("Eingabe erfolgreich\n");
}

// Funktion, um den NFA zu freigeben
void nfa_free(NFA* nfa) {
    for (int i = 0; i < nfa->num_states; i++) {
        free(nfa->states[i]->next_char);
        free(nfa->states[i]);
    }
    free(nfa->states);
    free(nfa);
}

// Beispiel usage
int main() {
    NFA* regex_nfa = nfa_new();
    nfa_init(regex_nfa);

    char regex[] = "a+b";
    nfa_construct(regex_nfa, regex);

    char input[] = "\u0001\u0002";

    nfa_parse(regex_nfa, input);

    nfa_free(regex_nfa);
    return 0;
}