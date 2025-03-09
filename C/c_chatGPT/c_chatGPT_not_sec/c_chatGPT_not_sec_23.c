#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define STACK_SIZE 256

// Bytecode definitions
typedef enum {
    OP_PUSH = 1,
    OP_ADD,
    OP_MUL,
    OP_POP,
    OP_HALT
} OpCode;

// Virtual Machine structure
typedef struct {
    int stack[STACK_SIZE];
    int sp; // Stack pointer
} VM;

// Initialize the VM
void initVM(VM *vm) {
    vm->sp = -1;
}

// Push a value onto the stack
void push(VM *vm, int value) {
    if (vm->sp < STACK_SIZE - 1) {
        vm->stack[++(vm->sp)] = value;
    } else {
        fprintf(stderr, "Error: Stack overflow\n");
        exit(EXIT_FAILURE);
    }
}

// Pop a value from the stack
int pop(VM *vm) {
    if (vm->sp >= 0) {
        return vm->stack[(vm->sp)--];
    } else {
        fprintf(stderr, "Error: Stack underflow\n");
        exit(EXIT_FAILURE);
    }
}

// Run the bytecode on the VM
void run(VM *vm, uint8_t *bytecode, size_t bytecode_length) {
    size_t ip = 0; // Instruction pointer

    while (ip < bytecode_length) {
        uint8_t instruction = bytecode[ip++];

        switch (instruction) {
            case OP_PUSH: {
                if (ip < bytecode_length) {
                    int value = bytecode[ip++];
                    push(vm, value);
                } else {
                    fprintf(stderr, "Error: Incomplete PUSH instruction\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case OP_ADD: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a + b);
                break;
            }
            case OP_MUL: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a * b);
                break;
            }
            case OP_POP: {
                pop(vm);
                break;
            }
            case OP_HALT: {
                return;
            }
            default: {
                fprintf(stderr, "Error: Unknown opcode %d\n", instruction);
                exit(EXIT_FAILURE);
            }
        }
    }
}

// Main function to test the VM
int main() {
    // Example bytecode: Push 5, Push 3, Add, Push 2, Multiply, Pop
    uint8_t bytecode[] = {
        OP_PUSH, 5,
        OP_PUSH, 3,
        OP_ADD,
        OP_PUSH, 2,
        OP_MUL,
        OP_POP,
        OP_HALT
    };

    VM vm;
    initVM(&vm);
    run(&vm, bytecode, sizeof(bytecode));

    return 0;
}
