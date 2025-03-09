#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MEMORY_POOL_SIZE 1024 * 1024  // 1 MB

typedef struct MemoryBlock {
    void *data;
    size_t size;
    int ref_count;
    bool marked;
    struct MemoryBlock *next;
} MemoryBlock;

typedef struct MemoryManager {
    MemoryBlock *head;
    void *memory_pool;
    size_t used_memory;
} MemoryManager;

MemoryManager *create_memory_manager() {
    MemoryManager *manager = (MemoryManager *)malloc(sizeof(MemoryManager));
    manager->memory_pool = malloc(MEMORY_POOL_SIZE);
    manager->used_memory = 0;
    manager->head = NULL;
    return manager;
}

void *custom_alloc(MemoryManager *manager, size_t size) {
    if (manager->used_memory + size > MEMORY_POOL_SIZE) {
        fprintf(stderr, "Memory pool exhausted\n");
        return NULL;
    }

    MemoryBlock *block = (MemoryBlock *)malloc(sizeof(MemoryBlock));
    block->data = manager->memory_pool + manager->used_memory;
    block->size = size;
    block->ref_count = 1;
    block->marked = false;
    block->next = manager->head;
    manager->head = block;

    manager->used_memory += size;
    return block->data;
}

void custom_free(MemoryManager *manager, void *ptr) {
    MemoryBlock **indirect = &manager->head;

    while (*indirect) {
        if ((*indirect)->data == ptr) {
            MemoryBlock *to_free = *indirect;
            *indirect = (*indirect)->next;
            free(to_free);
            return;
        }
        indirect = &(*indirect)->next;
    }

    fprintf(stderr, "Attempted to free unmanaged memory\n");
}

void increment_ref(MemoryManager *manager, void *ptr) {
    for (MemoryBlock *block = manager->head; block; block = block->next) {
        if (block->data == ptr) {
            block->ref_count++;
            return;
        }
    }
    fprintf(stderr, "Pointer not managed\n");
}

void decrement_ref(MemoryManager *manager, void *ptr) {
    for (MemoryBlock *block = manager->head; block; block = block->next) {
        if (block->data == ptr) {
            block->ref_count--;
            if (block->ref_count <= 0) {
                custom_free(manager, ptr);
            }
            return;
        }
    }
    fprintf(stderr, "Pointer not managed\n");
}

void mark(MemoryManager *manager, void *root) {
    for (MemoryBlock *block = manager->head; block; block = block->next) {
        if (block->data == root) {
            block->marked = true;
        }
    }
}

void sweep(MemoryManager *manager) {
    MemoryBlock **indirect = &manager->head;

    while (*indirect) {
        if (!(*indirect)->marked && (*indirect)->ref_count <= 0) {
            MemoryBlock *to_free = *indirect;
            *indirect = (*indirect)->next;
            free(to_free);
        } else {
            (*indirect)->marked = false;
            indirect = &(*indirect)->next;
        }
    }
}

void defragment(MemoryManager *manager) {
    // Simplistic defragmentation example
    void *current_position = manager->memory_pool;

    for (MemoryBlock *block = manager->head; block; block = block->next) {
        if (block->data != current_position) {
            memmove(current_position, block->data, block->size);
            block->data = current_position;
        }
        current_position += block->size;
    }
}

void check_memory_leaks(MemoryManager *manager) {
    for (MemoryBlock *block = manager->head; block; block = block->next) {
        if (block->ref_count > 0) {
            fprintf(stderr, "Memory leak detected: %p (%zu bytes, ref_count: %d)\n",
                    block->data, block->size, block->ref_count);
        }
    }
}

void destroy_memory_manager(MemoryManager *manager) {
    check_memory_leaks(manager);
    free(manager->memory_pool);
    while (manager->head) {
        MemoryBlock *to_free = manager->head;
        manager->head = manager->head->next;
        free(to_free);
    }
    free(manager);
}

int main() {
    MemoryManager *manager = create_memory_manager();

    void *ptr1 = custom_alloc(manager, 256);
    void *ptr2 = custom_alloc(manager, 128);
    increment_ref(manager, ptr1);
    decrement_ref(manager, ptr2);

    mark(manager, ptr1);  // Simulate root marking
    sweep(manager);
    defragment(manager);

    decrement_ref(manager, ptr1);
    destroy_memory_manager(manager);

    return 0;
}
