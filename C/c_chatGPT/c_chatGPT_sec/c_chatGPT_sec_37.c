#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// Constants for simulation
#define MEMORY_POOL_SIZE 1024 * 1024 // 1 MB

// Memory Block Structure
typedef struct MemoryBlock {
    size_t size;
    int is_free;
    struct MemoryBlock *next;
} MemoryBlock;

// Global memory pool
static char memory_pool[MEMORY_POOL_SIZE];
static MemoryBlock *free_list = NULL;
static pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;

// Logging function
void log_message(const char *message) {
    FILE *log_file = fopen("memory_compactor.log", "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

// Initialize the memory pool
void memory_init() {
    pthread_mutex_lock(&memory_mutex);
    free_list = (MemoryBlock *)memory_pool;
    free_list->size = MEMORY_POOL_SIZE - sizeof(MemoryBlock);
    free_list->is_free = 1;
    free_list->next = NULL;
    pthread_mutex_unlock(&memory_mutex);
    log_message("Memory pool initialized.");
}

// Memory allocation function
void *memory_alloc(size_t size) {
    pthread_mutex_lock(&memory_mutex);
    MemoryBlock *current = free_list;
    while (current) {
        if (current->is_free && current->size >= size) {
            size_t remaining_size = current->size - size;
            if (remaining_size > sizeof(MemoryBlock)) {
                MemoryBlock *new_block = (MemoryBlock *)((char *)current + sizeof(MemoryBlock) + size);
                new_block->size = remaining_size - sizeof(MemoryBlock);
                new_block->is_free = 1;
                new_block->next = current->next;
                current->next = new_block;
            }
            current->size = size;
            current->is_free = 0;
            pthread_mutex_unlock(&memory_mutex);
            log_message("Memory allocated.");
            return (void *)((char *)current + sizeof(MemoryBlock));
        }
        current = current->next;
    }
    pthread_mutex_unlock(&memory_mutex);
    log_message("Memory allocation failed.");
    return NULL;
}

// Free allocated memory
void memory_free(void *ptr) {
    if (!ptr) return;

    pthread_mutex_lock(&memory_mutex);
    MemoryBlock *block = (MemoryBlock *)((char *)ptr - sizeof(MemoryBlock));
    block->is_free = 1;
    log_message("Memory freed.");

    // Merge contiguous free blocks
    MemoryBlock *current = free_list;
    while (current) {
        if (current->is_free && current->next && current->next->is_free) {
            current->size += sizeof(MemoryBlock) + current->next->size;
            current->next = current->next->next;
            log_message("Memory blocks merged.");
        } else {
            current = current->next;
        }
    }
    pthread_mutex_unlock(&memory_mutex);
}

// Compact memory to reduce fragmentation
void memory_compact() {
    pthread_mutex_lock(&memory_mutex);
    MemoryBlock *current = free_list;
    char *end_of_pool = memory_pool + MEMORY_POOL_SIZE;
    char *compact_ptr = memory_pool;

    while (current) {
        if (!current->is_free) {
            size_t block_size = sizeof(MemoryBlock) + current->size;
            memmove(compact_ptr, current, block_size);
            current = (MemoryBlock *)(compact_ptr + block_size);
            compact_ptr += block_size;
        } else {
            current = current->next;
        }
    }

    size_t remaining_size = end_of_pool - compact_ptr;
    if (remaining_size > sizeof(MemoryBlock)) {
        MemoryBlock *new_free_block = (MemoryBlock *)compact_ptr;
        new_free_block->size = remaining_size - sizeof(MemoryBlock);
        new_free_block->is_free = 1;
        new_free_block->next = NULL;
        free_list = new_free_block;
    }

    pthread_mutex_unlock(&memory_mutex);
    log_message("Memory compacted.");
}

// Main function to demonstrate functionality
int main() {
    memory_init();

    void *ptr1 = memory_alloc(256);
    void *ptr2 = memory_alloc(128);
    memory_free(ptr1);
    memory_free(ptr2);

    memory_compact();

    return 0;
}
