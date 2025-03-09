#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define MAX_STACK_SIZE 100
#define MAX_INPUT_LENGTH 256
#define DECIMAL_PLACES 6

typedef struct {
    double items[MAX_STACK_SIZE];
    int top;
} Stack;

// Stack operations
void initStack(Stack *s) {
    s->top = -1;
}

int isFull(Stack *s) {
    return s->top >= MAX_STACK_SIZE - 1;
}

int isEmpty(Stack *s) {
    return s->top < 0;
}

int push(Stack *s, double value) {
    if (isFull(s)) {
        fprintf(stderr, "Error: Stack overflow\n");
        return 0;
    }
    s->items[++(s->top)] = value;
    return 1;
}

int pop(Stack *s, double *value) {
    if (isEmpty(s)) {
        fprintf(stderr, "Error: Stack underflow\n");
        return 0;
    }
    *value = s->items[(s->top)--];
    return 1;
}

// Input validation
int isValidOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

int isValidChar(char c) {
    return isdigit(c) || isspace(c) || isValidOperator(c) || c == '.' || c == '\n';
}

// Sanitize input
void sanitizeInput(char *input) {
    char *src = input;
    char *dst = input;
    
    while (*src) {
        if (isValidChar(*src)) {
            *dst = *src;
            dst++;
        }
        src++;
    }
    *dst = '\0';
}

// Perform operation
int performOperation(Stack *s, char operator) {
    double op2, op1, result;
    
    // Pop operands in correct order
    if (!pop(s, &op2) || !pop(s, &op1)) {
        return 0;
    }
    
    switch (operator) {
        case '+':
            result = op1 + op2;
            break;
        case '-':
            result = op1 - op2;
            break;
        case '*':
            result = op1 * op2;
            break;
        case '/':
            if (op2 == 0) {
                fprintf(stderr, "Error: Division by zero\n");
                return 0;
            }
            result = op1 / op2;
            break;
        default:
            fprintf(stderr, "Error: Invalid operator\n");
            return 0;
    }
    
    // Check for overflow/underflow
    if (result > DBL_MAX || result < -DBL_MAX) {
        fprintf(stderr, "Error: Result overflow\n");
        return 0;
    }
    
    return push(s, result);
}

// Evaluate expression
double evaluateExpression(const char *expr) {
    Stack stack;
    initStack(&stack);
    
    char *token = strtok((char *)expr, " \n\t");
    while (token != NULL) {
        if (isValidOperator(token[0]) && token[1] == '\0') {
            if (!performOperation(&stack, token[0])) {
                return 0;
            }
        } else {
            char *endptr;
            errno = 0;
            double value = strtod(token, &endptr);
            
            if (errno == ERANGE || endptr == token || *endptr != '\0') {
                fprintf(stderr, "Error: Invalid number format\n");
                return 0;
            }
            
            if (!push(&stack, value)) {
                return 0;
            }
        }
        token = strtok(NULL, " \n\t");
    }
    
    double result;
    if (!pop(&stack, &result) || !isEmpty(&stack)) {
        fprintf(stderr, "Error: Invalid expression format\n");
        return 0;
    }
    
    return result;
}

int main() {
    char input[MAX_INPUT_LENGTH];
    printf("Enter RPN expression (e.g., '5 3 + 2 *' for (5+3)*2):\n");
    
    while (fgets(input, sizeof(input), stdin)) {
        if (strlen(input) == 1 && input[0] == '\n') {
            break;
        }
        
        // Sanitize input
        sanitizeInput(input);
        
        // Evaluate expression
        double result = evaluateExpression(input);
        if (result != 0 || !errno) {
            printf("Result: %.6f\n", result);
        }
        
        printf("\nEnter another expression (empty line to quit):\n");
    }
    
    return 0;
}