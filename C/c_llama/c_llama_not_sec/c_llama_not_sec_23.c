#include <stdio.h>
#include <stdlib.h>

#define INSTRUCTIONS_LENGTH 4

// Typen und Funktionen für die Virtual Machine
typedef enum {
    OP_PUSH,
    OP_POP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_HALT
} Opcode;

typedef struct {
    unsigned char opcode;
    int value;
} Instruction;

void vm_init(void);
void execute_vm(Instruction *instructions, int length);

// Opcode-Funktionen
void opcodes[3]([[unsigned int]* value)] {
    case OP_PUSH:
        *(unsigned int*)value = 1;
        break;

    case OP_POP:
        // Pop das obere Element vom Stapel und speichern es in die Variable top_element
        unsigned int top_element;
        *(&top_element) = stack_top();
        break;

    case OP_ADD:
        // Addiere das obere und untere Element auf dem Stapel und füge das Ergebnis hinzu
        unsigned int top1, top2;
        *(&top1) = stack_top();
        *(&top2) = stack_pop();
        unsigned int result = top1 + top2;
        value(&result);
        break;

    case OP_SUB:
        // Subtrahiere das obere Element vom unteren auf dem Stapel und füge das Ergebnis hinzu
        unsigned int top1, top2;
        *(&top1) = stack_top();
        *(&top2) = stack_pop();
        unsigned int result = top1 - top2;
        value(&result);
        break;

    case OP_MUL:
        // Multipliziere das obere und untere Element auf dem Stapel und füge das Ergebnis hinzu
        unsigned int top1, top2;
        *(&top1) = stack_top();
        *(&top2) = stack_pop();
        unsigned int result = top1 * top2;
        value(&result);
        break;

    case OP_DIV:
        // Teile das obere Element vom unteren auf dem Stapel durch und füge das Ergebnis hinzu
        unsigned int top1, top2;
        *(&top1) = stack_top();
        *(&top2) = stack_pop();
        if (*(&top2) == 0) {
            printf("Division by zero!\n");
            exit(1);
        }
        unsigned int result = top1 / top2;
        value(&result);
        break;

    case OP_HALT:
        // Halt die VM
        printf("VM halting\n");
        exit(0);
}

// Stack-Funktionen
int* stack_top(void) {
    return &stack[stack_size - 1];
}

void stack_pop() {
    stack_size--;
}

int main() {
    Instruction instructions[] = {
        {OP_PUSH, 5},         // Füge 5 auf den Stapel
        {OP_ADD, NULL},       // Addiere das obere und untere Element auf dem Stapel
        {OP_SUB, NULL},       // Subtrahiere das obere Element vom unteren auf dem Stapel
        {OP_MUL, NULL}        // Multipliziere das obere und untere Element auf dem Stapel
    };

    vm_init();
    execute_vm(instructions, INSTRUCTIONS_LENGTH);
    return 0;
}

// Initialisierung der Stack-Variable
int stack[100];         // Initialisierte Menge an Variablen im Stapel
int stack_size = 0;     // Momentanige Anzahl der Elemente auf dem Stapel

void vm_init(void) {
    printf("VM initialisiert\n");
}

void execute_vm(Instruction *instructions, int length) {
    for (int i = 0; i < length; i++) {
        Instruction instruction = instructions[i];
        switch (instruction.opcode) {
            case OP_PUSH:
                *(unsigned int*)(&stack[instruction.value]) = stack_size;
                stack_size++;
                break;
            case OP_POP:
                *(&top_element) = stack_pop();
                break;
            case OP_ADD:
                opcodes[&top1, &top2, &result][0] = stack_top()[0];
                opcodes[&top2, &result][0] = stack_pop()[0];
                opcodes[&result][0] = *(&opcodes[&top1, &top2, &result]);
                break;
            case OP_SUB:
                opcodes[&top1, &top2, &result][0] = *(stack_top()[0]);
                opcodes[&top2, &result][0] = stack_pop()[0];
                opcodes[&result][0] = *(opcodes[&top1, &top2, &result])[0] - *(opcodes[&top2, 
&result])[0];
                break;
            case OP_MUL:
                opcodes[&top1, &top2, &result][0] = *(stack_top()[0]);
                opcodes[&top2, &result][0] = stack_pop()[0];
                opcodes[&result][0] = *(opcodes[&top1, &top2, &result])[0] * *(opcodes[&top2, 
&result])[0];
                break;
            case OP_DIV:
                opcodes[&top1, &top2, &result][0] = *(stack_top()[0]);
                opcodes[&top2, &result][0] = stack_pop()[0];
                if (*(opcodes[&top2, &result])[0] == 0) {
                    printf("Division by zero!\n");
                    exit(1);
                }
                opcodes[&result][0] = *(opcodes[&top1, &top2, &result])[0] / *(opcodes[&top2, 
&result]);
                break;
            case OP_HALT:
                return;
        }
    }
}
