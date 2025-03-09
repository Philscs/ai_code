#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STATES 100
#define MAX_TRANSITIONS 200

// Strukturen für den NFA
typedef struct Transition {
    int from;
    int to;
    char symbol;
} Transition;

typedef struct NFA {
    int num_states;
    int num_transitions;
    int start_state;
    int accept_state;
    Transition transitions[MAX_TRANSITIONS];
} NFA;

// Stack für die Konstruktion
typedef struct NFAStack {
    NFA* items[100];
    int top;
} NFAStack;

// Stack Operationen
void push(NFAStack* stack, NFA* nfa) {
    if (stack->top < 99) {
        stack->items[++stack->top] = nfa;
    }
}

NFA* pop(NFAStack* stack) {
    if (stack->top >= 0) {
        return stack->items[stack->top--];
    }
    return NULL;
}

// NFA Operationen
NFA* create_basic_nfa(char symbol) {
    NFA* nfa = (NFA*)malloc(sizeof(NFA));
    nfa->num_states = 2;
    nfa->num_transitions = 1;
    nfa->start_state = 0;
    nfa->accept_state = 1;
    
    nfa->transitions[0].from = 0;
    nfa->transitions[0].to = 1;
    nfa->transitions[0].symbol = symbol;
    
    return nfa;
}

NFA* concatenate(NFA* first, NFA* second) {
    NFA* result = (NFA*)malloc(sizeof(NFA));
    int offset = first->num_states;
    
    // Kopiere Übergänge vom ersten NFA
    memcpy(result->transitions, first->transitions, 
           first->num_transitions * sizeof(Transition));
    
    // Füge Übergänge vom zweiten NFA hinzu
    for (int i = 0; i < second->num_transitions; i++) {
        Transition t = second->transitions[i];
        result->transitions[first->num_transitions + i].from = t.from + offset - 1;
        result->transitions[first->num_transitions + i].to = t.to + offset - 1;
        result->transitions[first->num_transitions + i].symbol = t.symbol;
    }
    
    result->num_states = first->num_states + second->num_states - 1;
    result->num_transitions = first->num_transitions + second->num_transitions;
    result->start_state = first->start_state;
    result->accept_state = second->accept_state + offset - 1;
    
    return result;
}

NFA* union_nfa(NFA* first, NFA* second) {
    NFA* result = (NFA*)malloc(sizeof(NFA));
    int offset = first->num_states + 1;
    
    // Neuer Startzustand mit ε-Übergängen
    result->transitions[0].from = 0;
    result->transitions[0].to = first->start_state + 1;
    result->transitions[0].symbol = 'ε';
    
    result->transitions[1].from = 0;
    result->transitions[1].to = second->start_state + offset;
    result->transitions[1].symbol = 'ε';
    
    // Kopiere Übergänge vom ersten NFA
    for (int i = 0; i < first->num_transitions; i++) {
        Transition t = first->transitions[i];
        result->transitions[i + 2].from = t.from + 1;
        result->transitions[i + 2].to = t.to + 1;
        result->transitions[i + 2].symbol = t.symbol;
    }
    
    // Kopiere Übergänge vom zweiten NFA
    for (int i = 0; i < second->num_transitions; i++) {
        Transition t = second->transitions[i];
        result->transitions[i + 2 + first->num_transitions].from = t.from + offset;
        result->transitions[i + 2 + first->num_transitions].to = t.to + offset;
        result->transitions[i + 2 + first->num_transitions].symbol = t.symbol;
    }
    
    result->num_states = first->num_states + second->num_states + 2;
    result->num_transitions = first->num_transitions + second->num_transitions + 2;
    result->start_state = 0;
    result->accept_state = result->num_states - 1;
    
    return result;
}

NFA* kleene_star(NFA* nfa) {
    NFA* result = (NFA*)malloc(sizeof(NFA));
    
    // Neuer Start- und Akzeptanzzustand
    result->transitions[0].from = 0;
    result->transitions[0].to = 1;
    result->transitions[0].symbol = 'ε';
    
    // Kopiere original NFA Übergänge
    for (int i = 0; i < nfa->num_transitions; i++) {
        Transition t = nfa->transitions[i];
        result->transitions[i + 1].from = t.from + 1;
        result->transitions[i + 1].to = t.to + 1;
        result->transitions[i + 1].symbol = t.symbol;
    }
    
    // Füge Rückkopplungsübergang hinzu
    result->transitions[nfa->num_transitions + 1].from = nfa->accept_state + 1;
    result->transitions[nfa->num_transitions + 1].to = nfa->start_state + 1;
    result->transitions[nfa->num_transitions + 1].symbol = 'ε';
    
    // Verbinde mit neuem Akzeptanzzustand
    result->transitions[nfa->num_transitions + 2].from = 0;
    result->transitions[nfa->num_transitions + 2].to = nfa->num_states + 1;
    result->transitions[nfa->num_transitions + 2].symbol = 'ε';
    
    result->transitions[nfa->num_transitions + 3].from = nfa->accept_state + 1;
    result->transitions[nfa->num_transitions + 3].to = nfa->num_states + 1;
    result->transitions[nfa->num_transitions + 3].symbol = 'ε';
    
    result->num_states = nfa->num_states + 2;
    result->num_transitions = nfa->num_transitions + 4;
    result->start_state = 0;
    result->accept_state = nfa->num_states + 1;
    
    return result;
}

// Hauptfunktion zur NFA-Konstruktion
NFA* regex_to_nfa(const char* regex) {
    NFAStack stack = { .top = -1 };
    
    for (int i = 0; regex[i] != '\0'; i++) {
        if (regex[i] == '|') {
            NFA* second = pop(&stack);
            NFA* first = pop(&stack);
            push(&stack, union_nfa(first, second));
            free(first);
            free(second);
        }
        else if (regex[i] == '*') {
            NFA* nfa = pop(&stack);
            push(&stack, kleene_star(nfa));
            free(nfa);
        }
        else if (regex[i] == '(' || regex[i] == ')') {
            continue;
        }
        else {
            push(&stack, create_basic_nfa(regex[i]));
        }
    }
    
    // Verkette alle verbleibenden NFAs
    while (stack.top > 0) {
        NFA* second = pop(&stack);
        NFA* first = pop(&stack);
        push(&stack, concatenate(first, second));
        free(first);
        free(second);
    }
    
    return pop(&stack);
}

// Hilfsfunktion zum Ausdrucken des NFAs
void print_nfa(NFA* nfa) {
    printf("NFA mit %d Zuständen und %d Übergängen:\n", 
           nfa->num_states, nfa->num_transitions);
    printf("Startzustand: %d\n", nfa->start_state);
    printf("Akzeptanzzustand: %d\n", nfa->accept_state);
    
    printf("Übergänge:\n");
    for (int i = 0; i < nfa->num_transitions; i++) {
        printf("%d --%c--> %d\n", 
               nfa->transitions[i].from,
               nfa->transitions[i].symbol,
               nfa->transitions[i].to);
    }
}

// Beispiel für die Verwendung
int main() {
    const char* regex = "a(b|c)*";
    NFA* nfa = regex_to_nfa(regex);
    
    printf("NFA für den regulären Ausdruck '%s':\n", regex);
    print_nfa(nfa);
    
    free(nfa);
    return 0;
}