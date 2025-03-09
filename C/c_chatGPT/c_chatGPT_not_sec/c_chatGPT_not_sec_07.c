#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STATES 256
#define EPSILON '\0'

typedef struct State {
    struct State *out1;
    struct State *out2;
    char transition;
} State;

typedef struct Frag {
    State *start;
    State **out;
} Frag;

typedef struct Stack {
    Frag frags[MAX_STATES];
    int top;
} Stack;

void push(Stack *stack, Frag frag) {
    if (stack->top >= MAX_STATES) {
        fprintf(stderr, "Stack overflow\n");
        exit(1);
    }
    stack->frags[stack->top++] = frag;
}

Frag pop(Stack *stack) {
    if (stack->top == 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    return stack->frags[--stack->top];
}

State *create_state(char transition, State *out1, State *out2) {
    State *state = (State *)malloc(sizeof(State));
    if (!state) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    state->transition = transition;
    state->out1 = out1;
    state->out2 = out2;
    return state;
}

void patch(State **out, State *state) {
    *out = state;
}

State **list(State **out) {
    return out;
}

Frag fragment(State *start, State **out) {
    Frag frag = {start, out};
    return frag;
}

State *postfix_to_nfa(const char *postfix) {
    Stack stack = {.top = 0};
    for (const char *p = postfix; *p; p++) {
        switch (*p) {
        case '.': { // Concatenation
            Frag e2 = pop(&stack);
            Frag e1 = pop(&stack);
            patch(e1.out, e2.start);
            push(&stack, fragment(e1.start, e2.out));
            break;
        }
        case '|': { // Alternation
            Frag e2 = pop(&stack);
            Frag e1 = pop(&stack);
            State *state = create_state(EPSILON, e1.start, e2.start);
            push(&stack, fragment(state, list(e2.out)));
            break;
        }
        case '*': { // Kleene star
            Frag e = pop(&stack);
            State *state = create_state(EPSILON, e.start, NULL);
            patch(e.out, state);
            push(&stack, fragment(state, list(&state->out2)));
            break;
        }
        default: { // Literal characters
            State *state = create_state(*p, NULL, NULL);
            push(&stack, fragment(state, list(&state->out1)));
            break;
        }
        }
    }

    Frag nfa = pop(&stack);
    State *end_state = create_state(EPSILON, NULL, NULL);
    patch(nfa.out, end_state);
    return nfa.start;
}

void print_nfa(State *start) {
    printf("NFA:\n");
    State *queue[MAX_STATES];
    int visited[MAX_STATES] = {0};
    int front = 0, rear = 0;

    queue[rear++] = start;
    while (front < rear) {
        State *state = queue[front++];

        if (visited[(uintptr_t)state])
            continue;

        visited[(uintptr_t)state] = 1;
        printf("State %p: transition='%c', out1=%p, out2=%p\n",
               (void *)state, state->transition, (void *)state->out1, (void *)state->out2);

        if (state->out1 && !visited[(uintptr_t)state->out1])
            queue[rear++] = state->out1;
        if (state->out2 && !visited[(uintptr_t)state->out2])
            queue[rear++] = state->out2;
    }
}

int main() {
    const char *postfix = "ab.c|"; // Example: (a.b)|c in postfix notation
    State *nfa_start = postfix_to_nfa(postfix);
    print_nfa(nfa_start);
    return 0;
}
