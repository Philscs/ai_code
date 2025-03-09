#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Struktur für ein Speicherblock
typedef struct {
    size_t size;
    bool free;
} Block;

// Struktur für den Memory Allocator
typedef struct {
    Block* blocks;
    size_t numBlocks;
    size_t capacity;
} MemoryAllocator;

// Funktion zur Erstellung eines neuen Memory Allocators
MemoryAllocator* createAllocator(size_t capacity) {
    MemoryAllocator* allocator = malloc(sizeof(MemoryAllocator));
    allocator->blocks = malloc(sizeof(Block) * (capacity / sizeof(Block)));
    for (size_t i = 0; i < capacity; i++) {
        allocator->blocks[i].size = 1;
        allocator->blocks[i].free = true;
    }
    allocator->numBlocks = capacity;
    allocator->capacity = capacity;
    return allocator;
}

// Funktion zur Erstellung eines neuen Speicherblocks
void createBlock(MemoryAllocator* allocator, size_t size) {
    Block* block = &allocator->blocks[allocator->numBlocks];
    block->size = size;
    block->free = true;
    allocator->numBlocks++;
}

// Funktion für First-Fit-Allokation
void firstFit(MemoryAllocator* allocator, void** ptr, size_t size) {
    for (size_t i = 0; i < allocator->numBlocks; i++) {
        if (!allocator->blocks[i].free && allocator->blocks[i].size >= size) {
            *ptr = &allocator->blocks[i];
            allocator->blocks[i].free = false;
            return;
        }
    }
    createBlock(allocator, size);
}

// Funktion für Best-Fit-Allokation
void bestFit(MemoryAllocator* allocator, void** ptr, size_t size) {
    for (size_t i = 0; i < allocator->numBlocks; i++) {
        if (!allocator->blocks[i].free && allocator->blocks[i].size >= size) {
            // Coaleszen
            Block* nextBlock = &allocator->blocks[i + 1];
            while (nextBlock != NULL && !nextBlock->free && nextBlock->size > size - 
allocator->blocks[i].size) {
                nextBlock->size += allocator->blocks[i].size;
                allocator->blocks[i].size = nextBlock->size - size;
                i++;
                nextBlock = &allocator->blocks[i];
            }
            *ptr = &allocator->blocks[i];
            allocator->blocks[i].free = false;
            return;
        } else if (nextBlock != NULL && !nextBlock->free) {
            // Coaleszen
            Block* prevBlock = &allocator->blocks[i - 1];
            while (prevBlock != NULL && !prevBlock->free && nextBlock->size + 
allocator->blocks[i].size > size) {
                size_t newSize = nextBlock->size + allocator->blocks[i].size;
                nextBlock->size -= newSize - size;
                prevBlock->size += newSize - size;
                i--;
                prevBlock = &allocator->blocks[i];
            }
        }
    }
    createBlock(allocator, size);
}

// Funktion für Worst-Fit-Allokation
void worstFit(MemoryAllocator* allocator, void** ptr, size_t size) {
    Block* block = NULL;
    for (size_t i = 0; i < allocator->numBlocks; i++) {
        if (!allocator->blocks[i].free && allocator->blocks[i].size >= size) {
            block = &allocator->blocks[i];
            break;
        }
    }
    if (block == NULL) {
        createBlock(allocator, size);
        return;
    }

    // Coaleszen
    Block* nextBlock = &allocator->blocks[i + 1];
    while (nextBlock != NULL && !nextBlock->free && nextBlock->size > size - block->size) {
        nextBlock->size += block->size;
        block->size = nextBlock->size - size;
        i++;
        nextBlock = &allocator->blocks[i];
    }

    if (block == &allocator->blocks[i]) {
        block->free = false;
        *ptr = block;
    } else {
        Block* prevBlock = &allocator->blocks[i - 1];
        while (prevBlock != NULL && !prevBlock->free && nextBlock->size + block->size > size) {
            size_t newSize = nextBlock->size + block->size;
            nextBlock->size -= newSize - size;
            prevBlock->size += newSize - size;
            i--;
            prevBlock = &allocator->blocks[i];
        }
    }

}

// Funktion zur Freigabe des Speicherblock
void freeMemory(MemoryAllocator* allocator, void* ptr) {
    Block* block = (Block*)ptr;
    block->free = true;
    if (block == &allocator->blocks[allocator->numBlocks - 1]) {
        allocator->numBlocks--;
    }
}

int main() {
    MemoryAllocator* allocator = createAllocator(100);
    void* ptr = NULL;

    // Testen der Allokationsstrategien
    printf("First-Fit-Allokation: \n");
    firstFit(allocator, &ptr, 20);

    for (size_t i = 0; i < allocator->numBlocks; i++) {
        if (!allocator->blocks[i].free) {
            printf("%zu", allocator->blocks[i].size);
        }
    }

    printf("\n");

    printf("Best-Fit-Allokation: \n");
    bestFit(allocator, &ptr, 10);

    for (size_t i = 0; i < allocator->numBlocks; i++) {
        if (!allocator->blocks[i].free) {
            printf("%zu", allocator->blocks[i].size);
        }
    }

    printf("\n");

    printf("Worst-Fit-Allokation: \n");
    worstFit(allocator, &ptr, 15);

    for (size_t i = 0; i < allocator->numBlocks; i++) {
        if (!allocator->blocks[i].free) {
            printf("%zu", allocator->blocks[i].size);
        }
    }

    // Freigabe des Speicherblock
    freeMemory(allocator, ptr);

    return 0;
}
