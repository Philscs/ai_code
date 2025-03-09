#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_STACK_SIZE 100
#define MAX_EXPR_SIZE 256

typedef struct {
    double data[MAX_STACK_SIZE];
    int top;
} Stack;

// Stack-Funktionen
void initStack(Stack *stack) {
    stack->top = -1;
}

int isFull(Stack *stack) {
    return stack->top >= MAX_STACK_SIZE - 1;
}

int isEmpty(Stack *stack) {
    return stack->top == -1;
}

int push(Stack *stack, double value) {
    if (isFull(stack)) {
        fprintf(stderr, "Stack Overflow!\n");
        return 0;
    }
    stack->data[++stack->top] = value;
    return 1;
}

double pop(Stack *stack) {
    if (isEmpty(stack)) {
        fprintf(stderr, "Stack Underflow!\n");
        exit(EXIT_FAILURE);
    }
    return stack->data[stack->top--];
}

double peek(Stack *stack) {
    if (isEmpty(stack)) {
        fprintf(stderr, "Stack is empty!\n");
        exit(EXIT_FAILURE);
    }
    return stack->data[stack->top];
}

// Hilfsfunktion zur sicheren Eingabe
void safeInput(char *buffer, size_t size) {
    if (fgets(buffer, size, stdin) == NULL) {
        fprintf(stderr, "Fehler beim Lesen der Eingabe!\n");
        exit(EXIT_FAILURE);
    }
    buffer[strcspn(buffer, "\n")] = '\0'; // Entfernt das Newline-Zeichen
}

// Funktion zur Berechnung
int evaluateExpression(const char *expression, double *result) {
    Stack stack;
    initStack(&stack);

    char token[MAX_EXPR_SIZE];
    size_t i = 0, tokenIndex = 0;

    while (expression[i] != '\0') {
        if (isspace(expression[i])) {
            i++;
            continue;
        }

        if (isdigit(expression[i]) || expression[i] == '.') {
            token[tokenIndex++] = expression[i++];
            while (isdigit(expression[i]) || expression[i] == '.') {
                token[tokenIndex++] = expression[i++];
            }
            token[tokenIndex] = '\0';
            push(&stack, atof(token));
            tokenIndex = 0;
        } else if (expression[i] == '+' || expression[i] == '-' ||
                   expression[i] == '*' || expression[i] == '/') {
            if (stack.top < 1) {
                fprintf(stderr, "Ungültiger Ausdruck: Nicht genug Operanden.\n");
                return 0;
            }

            double b = pop(&stack);
            double a = pop(&stack);

            switch (expression[i]) {
                case '+':
                    push(&stack, a + b);
                    break;
                case '-':
                    push(&stack, a - b);
                    break;
                case '*':
                    push(&stack, a * b);
                    break;
                case '/':
                    if (b == 0) {
                        fprintf(stderr, "Fehler: Division durch Null!\n");
                        return 0;
                    }
                    push(&stack, a / b);
                    break;
                default:
                    fprintf(stderr, "Unbekannter Operator: %c\n", expression[i]);
                    return 0;
            }
            i++;
        } else {
            fprintf(stderr, "Ungültiges Zeichen im Ausdruck: %c\n", expression[i]);
            return 0;
        }
    }

    if (stack.top != 0) {
        fprintf(stderr, "Ungültiger Ausdruck: Zu viele Operanden.\n");
        return 0;
    }

    *result = pop(&stack);
    return 1;
}

int main() {
    char input[MAX_EXPR_SIZE];
    double result;

    printf("Geben Sie einen mathematischen Ausdruck in umgekehrter polnischer Notation (RPN) ein: ");
    safeInput(input, sizeof(input));

    if (strpbrk(input, ";|&><")) {
        fprintf(stderr, "Potentieller Injection-Angriff erkannt!\n");
        return EXIT_FAILURE;
    }

    if (evaluateExpression(input, &result)) {
        printf("Ergebnis: %.2f\n", result);
    } else {
        fprintf(stderr, "Fehler bei der Berechnung des Ausdrucks.\n");
    }

    return 0;
}
