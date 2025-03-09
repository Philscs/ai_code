#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

// Define the block size and number of blocks
#define BLOCK_SIZE 64
#define NUM_BLOCKS 100

// Structure for each block
typedef struct Block {
    bool is_allocated;
    struct Block* next;
    struct Block* prev;
    uint8_t data[BLOCK_SIZE];
} Block;

// Structure for the allocator
typedef struct Allocator {
    Block* head;
    pthread_mutex_t lock;
} Allocator;

// Global allocator instance
Allocator allocator;

// Initialize the allocator
void allocator_init() {
    allocator.head = NULL;
    pthread_mutex_init(&allocator.lock, NULL);
}

// Allocate a block of memory
void* allocator_alloc() {
    pthread_mutex_lock(&allocator.lock);

    // Find a free block
    Block* block = allocator.head;
    while (block != NULL && block->is_allocated) {
        block = block->next;
    }

    // If no free block found, return NULL
    if (block == NULL) {
        pthread_mutex_unlock(&allocator.lock);
        return NULL;
    }

    // Mark the block as allocated
    block->is_allocated = true;

    pthread_mutex_unlock(&allocator.lock);
    return block->data;
}

// Free a block of memory
void allocator_free(void* ptr) {
    pthread_mutex_lock(&allocator.lock);

    // Find the block corresponding to the pointer
    Block* block = (Block*)((uint8_t*)ptr - offsetof(Block, data));

    // Mark the block as free
    block->is_allocated = false;

    pthread_mutex_unlock(&allocator.lock);
}

// Cleanup the allocator
void allocator_cleanup() {
    pthread_mutex_destroy(&allocator.lock);
}

int main() {
    // Initialize the allocator
    allocator_init();

    // Allocate and free some blocks
    void* ptr1 = allocator_alloc();
    void* ptr2 = allocator_alloc();
    void* ptr3 = allocator_alloc();

    allocator_free(ptr2);

    // Cleanup the allocator
    allocator_cleanup();

    return 0;
}
