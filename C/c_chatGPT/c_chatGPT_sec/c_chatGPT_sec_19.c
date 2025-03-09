#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#define BLOCK_SIZE 64   // Größe eines Blocks
#define MEMORY_SIZE 1024 // Gesamtspeichergröße
#define ALIGNMENT 8     // Speicheralignment
#define POISON_BYTE 0xAA // Byte für Memory Poisoning

// Header für jeden Speicherblock
typedef struct BlockHeader {
    size_t size;           // Größe des Blocks
    int is_free;           // Block frei oder belegt
    struct BlockHeader* next; // Zeiger auf nächsten Block
} BlockHeader;

static uint8_t memory[MEMORY_SIZE]; // Der Speicherpool
static BlockHeader* free_list = NULL; // Freiliste
static pthread_mutex_t alloc_lock = PTHREAD_MUTEX_INITIALIZER;

// Hilfsfunktion: Align-Größe berechnen
size_t align(size_t size) {
    return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

// Speicher initialisieren
void initialize_memory() {
    free_list = (BlockHeader*)memory;
    free_list->size = MEMORY_SIZE - sizeof(BlockHeader);
    free_list->is_free = 1;
    free_list->next = NULL;
    memset((uint8_t*)free_list + sizeof(BlockHeader), POISON_BYTE, free_list->size); // Memory Poisoning
}

// Block splitten
void split_block(BlockHeader* block, size_t size) {
    BlockHeader* new_block = (BlockHeader*)((uint8_t*)block + sizeof(BlockHeader) + size);
    new_block->size = block->size - size - sizeof(BlockHeader);
    new_block->is_free = 1;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;

    memset((uint8_t*)new_block + sizeof(BlockHeader), POISON_BYTE, new_block->size); // Memory Poisoning
}

// Speicher anfordern
void* custom_malloc(size_t size) {
    pthread_mutex_lock(&alloc_lock);

    size = align(size);
    BlockHeader* current = free_list;
    BlockHeader* prev = NULL;

    while (current) {
        if (current->is_free && current->size >= size) {
            if (current->size >= size + sizeof(BlockHeader) + ALIGNMENT) {
                split_block(current, size);
            }
            current->is_free = 0;
            memset((uint8_t*)current + sizeof(BlockHeader), 0, size); // Use-After-Free Detection

            pthread_mutex_unlock(&alloc_lock);
            return (uint8_t*)current + sizeof(BlockHeader);
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&alloc_lock);
    return NULL; // Kein passender Block gefunden
}

// Speicher freigeben
void custom_free(void* ptr) {
    if (!ptr) return;

    pthread_mutex_lock(&alloc_lock);

    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    block->is_free = 1;
    memset((uint8_t*)block + sizeof(BlockHeader), POISON_BYTE, block->size); // Memory Poisoning

    // Blöcke zusammenfügen
    BlockHeader* current = free_list;
    while (current) {
        if (current->is_free && (uint8_t*)current + sizeof(BlockHeader) + current->size == (uint8_t*)block) {
            current->size += sizeof(BlockHeader) + block->size;
            current->next = block->next;
            block = current;
        } else if (block->is_free && (uint8_t*)block + sizeof(BlockHeader) + block->size == (uint8_t*)current) {
            block->size += sizeof(BlockHeader) + current->size;
            block->next = current->next;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&alloc_lock);
}

int main() {
    initialize_memory();

    void* ptr1 = custom_malloc(50);
    void* ptr2 = custom_malloc(100);

    custom_free(ptr1);
    custom_free(ptr2);

    return 0;
}
