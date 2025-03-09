#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

// Define the maximum number of memory blocks in the pool
#define MAX_BLOCKS 100

// Define the size of each memory block
#define BLOCK_SIZE 1024

// Structure to represent a memory block
typedef struct {
    bool allocated;
    void* address;
} MemoryBlock;

// Structure to represent the memory pool
typedef struct {
    MemoryBlock blocks[MAX_BLOCKS];
    pthread_mutex_t lock;
} MemoryPool;

// Initialize the memory pool
void initMemoryPool(MemoryPool* pool) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        pool->blocks[i].allocated = false;
        pool->blocks[i].address = malloc(BLOCK_SIZE);
    }
    pthread_mutex_init(&pool->lock, NULL);
}

// Allocate a memory block from the pool
void* allocateMemory(MemoryPool* pool) {
    pthread_mutex_lock(&pool->lock);
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!pool->blocks[i].allocated) {
            pool->blocks[i].allocated = true;
            pthread_mutex_unlock(&pool->lock);
            return pool->blocks[i].address;
        }
    }
    pthread_mutex_unlock(&pool->lock);
    return NULL; // No available memory block
}

// Free a memory block in the pool
void freeMemory(MemoryPool* pool, void* address) {
    pthread_mutex_lock(&pool->lock);
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (pool->blocks[i].address == address) {
            pool->blocks[i].allocated = false;
            break;
        }
    }
    pthread_mutex_unlock(&pool->lock);
}

// Destroy the memory pool
void destroyMemoryPool(MemoryPool* pool) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        free(pool->blocks[i].address);
    }
    pthread_mutex_destroy(&pool->lock);
}

int main() {
    MemoryPool pool;
    initMemoryPool(&pool);

    // Test the memory pool
    void* block1 = allocateMemory(&pool);
    void* block2 = allocateMemory(&pool);
    void* block3 = allocateMemory(&pool);

    printf("Block 1: %p\n", block1);
    printf("Block 2: %p\n", block2);
    printf("Block 3: %p\n", block3);

    freeMemory(&pool, block2);

    void* block4 = allocateMemory(&pool);
    printf("Block 4: %p\n", block4);

    destroyMemoryPool(&pool);

    return 0;
}
