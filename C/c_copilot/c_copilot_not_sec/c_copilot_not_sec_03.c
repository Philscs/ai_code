#include <stdio.h>
#include <stdlib.h>

// Define the memory block size
#define MEMORY_SIZE 1024

// Define the allocation strategies
typedef enum {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
} AllocationStrategy;

// Define the memory block structure
typedef struct {
    int size;
    int is_free;
    struct MemoryBlock* next;
} MemoryBlock;

// Global memory block
MemoryBlock* memory = NULL;

// Function to initialize the memory allocator
void initialize() {
    memory = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    memory->size = MEMORY_SIZE - sizeof(MemoryBlock);
    memory->is_free = 1;
    memory->next = NULL;
}

// Function to allocate memory using First Fit strategy
void* allocate_first_fit(int size) {
    MemoryBlock* current = memory;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            if (current->size > size + sizeof(MemoryBlock)) {
                MemoryBlock* new_block = (MemoryBlock*)((char*)current + sizeof(MemoryBlock) + size);
                new_block->size = current->size - size - sizeof(MemoryBlock);
                new_block->is_free = 1;
                new_block->next = current->next;
                current->size = size;
                current->is_free = 0;
                current->next = new_block;
            } else {
                current->is_free = 0;
            }
            return (void*)((char*)current + sizeof(MemoryBlock));
        }
        current = current->next;
    }
    return NULL;
}

// Function to allocate memory using Best Fit strategy
void* allocate_best_fit(int size) {
    // TODO: Implement Best Fit strategy
    return NULL;
}

// Function to allocate memory using Worst Fit strategy
void* allocate_worst_fit(int size) {
    // TODO: Implement Worst Fit strategy
    return NULL;
}

// Function to deallocate memory and perform coalescing
void deallocate(void* ptr) {
    MemoryBlock* block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
    block->is_free = 1;

    // Perform coalescing
    MemoryBlock* current = memory;
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += current->next->size + sizeof(MemoryBlock);
            current->next = current->next->next;
        }
        current = current->next;
    }
}

int main() {
    // Initialize the memory allocator
    initialize();

    // Allocate memory using First Fit strategy
    void* ptr1 = allocate_first_fit(100);
    void* ptr2 = allocate_first_fit(200);
    void* ptr3 = allocate_first_fit(300);

    // Deallocate memory
    deallocate(ptr2);

    // Allocate memory using First Fit strategy
    void* ptr4 = allocate_first_fit(150);

    // Deallocate memory
    deallocate(ptr1);
    deallocate(ptr3);
    deallocate(ptr4);

    // Free the memory allocator
    free(memory);

    return 0;
}
