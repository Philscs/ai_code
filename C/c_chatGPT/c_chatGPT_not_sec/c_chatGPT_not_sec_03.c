#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 1024

typedef enum { FIRST_FIT, BEST_FIT, WORST_FIT } AllocationStrategy;

typedef struct Block {
    size_t size;
    int free;
    struct Block* next;
} Block;

static char memory[MEMORY_SIZE];
static Block* freeList = (Block*)memory;
static AllocationStrategy strategy = FIRST_FIT;

void init_allocator(AllocationStrategy alloc_strategy) {
    strategy = alloc_strategy;
    freeList->size = MEMORY_SIZE - sizeof(Block);
    freeList->free = 1;
    freeList->next = NULL;
}

void split_block(Block* fitting_block, size_t size) {
    Block* new_block = (Block*)((char*)fitting_block + sizeof(Block) + size);
    new_block->size = fitting_block->size - size - sizeof(Block);
    new_block->free = 1;
    new_block->next = fitting_block->next;

    fitting_block->size = size;
    fitting_block->free = 0;
    fitting_block->next = new_block;
}

void* allocate(size_t size) {
    Block* current = freeList;
    Block* best_block = NULL;

    while (current) {
        if (current->free && current->size >= size) {
            if (strategy == FIRST_FIT) {
                if (current->size > size + sizeof(Block)) {
                    split_block(current, size);
                }
                current->free = 0;
                return (void*)((char*)current + sizeof(Block));
            } else if (strategy == BEST_FIT) {
                if (!best_block || current->size < best_block->size) {
                    best_block = current;
                }
            } else if (strategy == WORST_FIT) {
                if (!best_block || current->size > best_block->size) {
                    best_block = current;
                }
            }
        }
        current = current->next;
    }

    if (best_block) {
        if (best_block->size > size + sizeof(Block)) {
            split_block(best_block, size);
        }
        best_block->free = 0;
        return (void*)((char*)best_block + sizeof(Block));
    }

    return NULL;
}

void coalesce() {
    Block* current = freeList;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += current->next->size + sizeof(Block);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void deallocate(void* ptr) {
    if (!ptr) return;

    Block* block_to_free = (Block*)((char*)ptr - sizeof(Block));
    block_to_free->free = 1;
    coalesce();
}

void print_memory() {
    Block* current = freeList;
    printf("Memory State:\n");
    while (current) {
        printf("Block at %p, Size: %zu, Free: %d\n", (void*)current, current->size, current->free);
        current = current->next;
    }
}

int main() {
    init_allocator(FIRST_FIT);

    void* p1 = allocate(200);
    void* p2 = allocate(300);
    print_memory();

    deallocate(p1);
    print_memory();

    void* p3 = allocate(100);
    print_memory();

    deallocate(p2);
    deallocate(p3);
    print_memory();

    return 0;
}
