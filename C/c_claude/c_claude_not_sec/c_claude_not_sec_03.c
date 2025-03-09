#include <stdio.h>
#include <stdbool.h>

// Definiere die verschiedenen Allokationsstrategien
typedef enum {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
} AllocationStrategy;

// Struktur für einen Speicherblock
typedef struct Block {
    size_t size;
    bool is_free;
    struct Block* next;
    struct Block* prev;
} Block;

// Hauptstruktur des Memory Allocators
typedef struct {
    Block* head;
    AllocationStrategy strategy;
    size_t total_size;
    size_t used_size;
} MemoryAllocator;

// Initialisiere den Allocator
MemoryAllocator* init_allocator(size_t size, AllocationStrategy strategy) {
    MemoryAllocator* allocator = (MemoryAllocator*)malloc(sizeof(MemoryAllocator));
    if (!allocator) return NULL;

    // Erstelle den initialen freien Block
    Block* initial_block = (Block*)malloc(sizeof(Block) + size);
    if (!initial_block) {
        free(allocator);
        return NULL;
    }

    initial_block->size = size;
    initial_block->is_free = true;
    initial_block->next = NULL;
    initial_block->prev = NULL;

    allocator->head = initial_block;
    allocator->strategy = strategy;
    allocator->total_size = size;
    allocator->used_size = 0;

    return allocator;
}

// Finde einen passenden Block je nach Strategie
Block* find_block(MemoryAllocator* allocator, size_t size) {
    Block* current = allocator->head;
    Block* best_block = NULL;
    size_t best_size = SIZE_MAX;
    size_t worst_size = 0;

    switch (allocator->strategy) {
        case FIRST_FIT:
            while (current) {
                if (current->is_free && current->size >= size) {
                    return current;
                }
                current = current->next;
            }
            break;

        case BEST_FIT:
            while (current) {
                if (current->is_free && current->size >= size) {
                    if (current->size < best_size) {
                        best_block = current;
                        best_size = current->size;
                    }
                }
                current = current->next;
            }
            return best_block;

        case WORST_FIT:
            while (current) {
                if (current->is_free && current->size >= size) {
                    if (current->size > worst_size) {
                        best_block = current;
                        worst_size = current->size;
                    }
                }
                current = current->next;
            }
            return best_block;
    }
    return NULL;
}

// Implementiere Coalescing von freien Blöcken
void coalesce(MemoryAllocator* allocator, Block* block) {
    // Verschmelze mit nachfolgendem Block falls möglich
    if (block->next && block->next->is_free) {
        block->size += block->next->size + sizeof(Block);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Verschmelze mit vorherigem Block falls möglich
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size + sizeof(Block);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
}

// Allokiere Speicher
void* mem_alloc(MemoryAllocator* allocator, size_t size) {
    Block* block = find_block(allocator, size);
    if (!block) return NULL;

    // Wenn der Block größer ist als benötigt, teile ihn
    if (block->size > size + sizeof(Block) + 32) { // 32 Bytes Minimum für einen neuen Block
        Block* new_block = (Block*)((char*)block + sizeof(Block) + size);
        new_block->size = block->size - size - sizeof(Block);
        new_block->is_free = true;
        new_block->next = block->next;
        new_block->prev = block;
        
        block->size = size;
        block->next = new_block;
        
        if (new_block->next) {
            new_block->next->prev = new_block;
        }
    }

    block->is_free = false;
    allocator->used_size += block->size;
    
    return (void*)((char*)block + sizeof(Block));
}

// Gebe Speicher frei
void mem_free(MemoryAllocator* allocator, void* ptr) {
    if (!ptr) return;

    Block* block = (Block*)((char*)ptr - sizeof(Block));
    block->is_free = true;
    allocator->used_size -= block->size;

    coalesce(allocator, block);
}

// Gebe den Allocator frei
void destroy_allocator(MemoryAllocator* allocator) {
    if (!allocator) return;
    free(allocator->head);
    free(allocator);
}

// Beispielnutzung
void print_memory_status(MemoryAllocator* allocator) {
    printf("\nMemory Status:\n");
    printf("Total Size: %zu\n", allocator->total_size);
    printf("Used Size: %zu\n", allocator->used_size);
    printf("Free Size: %zu\n", allocator->total_size - allocator->used_size);
    
    Block* current = allocator->head;
    int block_count = 0;
    while (current) {
        printf("Block %d: Size=%zu, Free=%d\n", 
               block_count++, current->size, current->is_free);
        current = current->next;
    }
}

// Beispiel main-Funktion zur Demonstration
int main() {
    MemoryAllocator* allocator = init_allocator(1024, BEST_FIT);
    
    // Allokiere einige Blöcke
    void* ptr1 = mem_alloc(allocator, 128);
    void* ptr2 = mem_alloc(allocator, 256);
    void* ptr3 = mem_alloc(allocator, 64);
    
    print_memory_status(allocator);
    
    // Freigabe und Coalescing Test
    mem_free(allocator, ptr2);
    mem_free(allocator, ptr3);
    
    print_memory_status(allocator);
    
    destroy_allocator(allocator);
    return 0;
}