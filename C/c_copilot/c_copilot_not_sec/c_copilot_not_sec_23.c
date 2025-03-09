#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE 100

typedef struct {
    int stack[STACK_SIZE];
    int sp; // stack pointer
} VM;

void initVM(VM* vm) {
    vm->sp = -1;
}

void push(VM* vm, int value) {
    if (vm->sp < STACK_SIZE - 1) {
        vm->stack[++vm->sp] = value;
    } else {
        printf("Stack overflow!\n");
        exit(1);
    }
}

int pop(VM* vm) {
    if (vm->sp >= 0) {
        return vm->stack[vm->sp--];
    } else {
        printf("Stack underflow!\n");
        exit(1);
    }
}

void interpret(VM* vm, int* bytecode, int size) {
    int ip = 0; // instruction pointer

    while (ip < size) {
        int opcode = bytecode[ip++];

        switch (opcode) {
            case 0: // PUSH
                push(vm, bytecode[ip++]);
                break;
            case 1: // ADD
                push(vm, pop(vm) + pop(vm));
                break;
            case 2: // SUB
                push(vm, pop(vm) - pop(vm));
                break;
            case 3: // MUL
                push(vm, pop(vm) * pop(vm));
                break;
            case 4: // DIV
                push(vm, pop(vm) / pop(vm));
                break;
            case 5: // PRINT
                printf("%d\n", pop(vm));
                break;
            case 6: // HALT
                return;
            default:
                printf("Invalid opcode: %d\n", opcode);
                exit(1);
        }
    }
}

int main() {
    VM vm;
    initVM(&vm);

    int bytecode[] = {0, 42, 0, 7, 1, 5, 5, 6}; // PUSH 42, PUSH 7, ADD, PRINT, PRINT, HALT
    int size = sizeof(bytecode) / sizeof(bytecode[0]);

    interpret(&vm, bytecode, size);

    return 0;
}
