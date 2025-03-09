#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Konstanten für die VM
#define STACK_SIZE 256
#define CODE_SIZE 256

// Opcodes für unsere VM
typedef enum {
    PUSH,   // Wert auf Stack legen
    POP,    // Wert vom Stack entfernen
    ADD,    // Zwei Werte addieren
    SUB,    // Zwei Werte subtrahieren
    MUL,    // Zwei Werte multiplizieren
    DIV,    // Zwei Werte dividieren
    PRINT,  // Obersten Wert ausgeben
    HALT    // Programm beenden
} OpCode;

// Struktur für unsere VM
typedef struct {
    int32_t stack[STACK_SIZE];    // Stack für Operanden
    uint8_t code[CODE_SIZE];      // Programmspeicher
    int32_t stack_ptr;            // Stack-Pointer
    int32_t program_counter;      // Programm-Counter
    int running;                  // Status der VM
} VM;

// VM initialisieren
VM* vm_create() {
    VM* vm = (VM*)malloc(sizeof(VM));
    vm->stack_ptr = 0;
    vm->program_counter = 0;
    vm->running = 1;
    return vm;
}

// VM aufräumen
void vm_destroy(VM* vm) {
    free(vm);
}

// Wert auf den Stack legen
void vm_push(VM* vm, int32_t value) {
    if (vm->stack_ptr >= STACK_SIZE) {
        printf("Error: Stack overflow\n");
        vm->running = 0;
        return;
    }
    vm->stack[vm->stack_ptr++] = value;
}

// Wert vom Stack holen
int32_t vm_pop(VM* vm) {
    if (vm->stack_ptr <= 0) {
        printf("Error: Stack underflow\n");
        vm->running = 0;
        return 0;
    }
    return vm->stack[--vm->stack_ptr];
}

// Einen Befehl ausführen
void vm_execute(VM* vm, uint8_t opcode) {
    int32_t a, b;
    
    switch (opcode) {
        case PUSH:
            // Nächstes Byte als Wert interpretieren
            vm_push(vm, vm->code[++vm->program_counter]);
            break;
            
        case POP:
            vm_pop(vm);
            break;
            
        case ADD:
            b = vm_pop(vm);
            a = vm_pop(vm);
            vm_push(vm, a + b);
            break;
            
        case SUB:
            b = vm_pop(vm);
            a = vm_pop(vm);
            vm_push(vm, a - b);
            break;
            
        case MUL:
            b = vm_pop(vm);
            a = vm_pop(vm);
            vm_push(vm, a * b);
            break;
            
        case DIV:
            b = vm_pop(vm);
            if (b == 0) {
                printf("Error: Division by zero\n");
                vm->running = 0;
                return;
            }
            a = vm_pop(vm);
            vm_push(vm, a / b);
            break;
            
        case PRINT:
            printf("Output: %d\n", vm_pop(vm));
            break;
            
        case HALT:
            vm->running = 0;
            break;
            
        default:
            printf("Error: Unknown opcode %d\n", opcode);
            vm->running = 0;
    }
}

// Hauptschleife der VM
void vm_run(VM* vm) {
    while (vm->running && vm->program_counter < CODE_SIZE) {
        uint8_t opcode = vm->code[vm->program_counter];
        vm_execute(vm, opcode);
        vm->program_counter++;
    }
}

// Beispielprogramm laden
void load_test_program(VM* vm) {
    // Programm: PUSH 5, PUSH 3, ADD, PRINT, HALT
    int pc = 0;
    vm->code[pc++] = PUSH;
    vm->code[pc++] = 5;    // Erster Operand
    vm->code[pc++] = PUSH;
    vm->code[pc++] = 3;    // Zweiter Operand
    vm->code[pc++] = ADD;  // Addition
    vm->code[pc++] = PRINT;// Ergebnis ausgeben
    vm->code[pc++] = HALT; // Programm beenden
}

int main() {
    // VM erstellen und Testprogramm laden
    VM* vm = vm_create();
    load_test_program(vm);
    
    // VM ausführen
    printf("Starting VM...\n");
    vm_run(vm);
    
    // Aufräumen
    vm_destroy(vm);
    return 0;
}