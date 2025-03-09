#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_STACK_SIZE 100

typedef struct {
    int data[MAX_STACK_SIZE];
    int top;
} Stack;

void initialize(Stack* stack) {
    stack->top = -1;
}

bool isFull(Stack* stack) {
    return stack->top == MAX_STACK_SIZE - 1;
}

bool isEmpty(Stack* stack) {
    return stack->top == -1;
}

void push(Stack* stack, int value) {
    if (isFull(stack)) {
        printf("Stack overflow!\n");
        exit(1);
    }
    stack->data[++stack->top] = value;
}

int pop(Stack* stack) {
    if (isEmpty(stack)) {
        printf("Stack underflow!\n");
        exit(1);
    }
    return stack->data[stack->top--];
}

int evaluateExpression(const char* expression) {
    Stack stack;
    initialize(&stack);

    int i = 0;
    while (expression[i] != '\0') {
        if (expression[i] >= '0' && expression[i] <= '9') {
            int num = 0;
            while (expression[i] >= '0' && expression[i] <= '9') {
                num = num * 10 + (expression[i] - '0');
                i++;
            }
            push(&stack, num);
        } else if (expression[i] == '+' || expression[i] == '-' || expression[i] == '*' || expression[i] == '/') {
            if (isEmpty(&stack)) {
                printf("Invalid expression!\n");
                exit(1);
            }
            int operand2 = pop(&stack);
            if (isEmpty(&stack)) {
                printf("Invalid expression!\n");
                exit(1);
            }
            int operand1 = pop(&stack);

            int result;
            switch (expression[i]) {
                case '+':
                    result = operand1 + operand2;
                    break;
                case '-':
                    result = operand1 - operand2;
                    break;
                case '*':
                    result = operand1 * operand2;
                    break;
                case '/':
                    if (operand2 == 0) {
                        printf("Division by zero!\n");
                        exit(1);
                    }
                    result = operand1 / operand2;
                    break;
            }
            push(&stack, result);
            i++;
        } else {
            printf("Invalid character in expression!\n");
            exit(1);
        }
    }

    if (isEmpty(&stack)) {
        printf("Invalid expression!\n");
        exit(1);
    }
    return pop(&stack);
}

int main() {
    char expression[100];
    printf("Enter an expression: ");
    scanf("%s", expression);

    int result = evaluateExpression(expression);
    printf("Result: %d\n", result);

    return 0;
}
