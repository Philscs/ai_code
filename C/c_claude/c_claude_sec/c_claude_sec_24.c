#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>

// Konstanten für den Memory Pool
#define POOL_SIZE (1024 * 1024)  // 1MB Pool
#define MIN_BLOCK_SIZE 16        // Minimale Blockgröße
#define MAX_BLOCKS 1024          // Maximale Anzahl Blöcke
#define MAGIC_NUMBER 0xDEADBEEF  // Für Error Detection

// Block Header Struktur
typedef struct BlockHeader {
    uint32_t size;           // Größe des Blocks
    bool used;               // Block in Verwendung?
    uint32_t magic;          // Magic Number für Error Detection
    uint32_t checksum;       // Für Korruptionserkennung
    struct BlockHeader* next;
} BlockHeader;

// Memory Pool Struktur
typedef struct {
    uint8_t* memory;                 // Pointer auf den Speicherbereich
    size_t total_size;               // Gesamtgröße des Pools
    BlockHeader* free_list;          // Liste der freien Blöcke
    pthread_mutex_t mutex;           // Mutex für Thread Safety
    size_t allocated_blocks;         // Anzahl allozierter Blöcke
    uint32_t protection_guard;       // Memory Protection Guard
} MemoryPool;

// Hilfsfunktionen für Error Detection
static uint32_t calculate_checksum(BlockHeader* block) {
    uint32_t sum = 0;
    uint8_t* ptr = (uint8_t*)block;
    for (size_t i = 0; i < sizeof(BlockHeader) - sizeof(uint32_t); i++) {
        sum += ptr[i];
    }
    return sum;
}

static bool validate_block(BlockHeader* block) {
    if (block->magic != MAGIC_NUMBER) {
        return false;
    }
    uint32_t current_checksum = block->checksum;
    block->checksum = 0;
    uint32_t calculated_checksum = calculate_checksum(block);
    block->checksum = current_checksum;
    return current_checksum == calculated_checksum;
}

// Memory Pool Initialisierung
MemoryPool* create_memory_pool() {
    MemoryPool* pool = (MemoryPool*)malloc(sizeof(MemoryPool));
    if (!pool) return NULL;

    // Hauptspeicher allozieren
    pool->memory = (uint8_t*)aligned_alloc(16, POOL_SIZE);
    if (!pool->memory) {
        free(pool);
        return NULL;
    }

    // Initialisierung
    pool->total_size = POOL_SIZE;
    pool->allocated_blocks = 0;
    pool->protection_guard = MAGIC_NUMBER;
    
    // Mutex initialisieren
    pthread_mutex_init(&pool->mutex, NULL);

    // Ersten Block initialisieren
    BlockHeader* first_block = (BlockHeader*)pool->memory;
    first_block->size = POOL_SIZE - sizeof(BlockHeader);
    first_block->used = false;
    first_block->magic = MAGIC_NUMBER;
    first_block->next = NULL;
    first_block->checksum = calculate_checksum(first_block);

    pool->free_list = first_block;

    return pool;
}

// Memory Allocation
void* pool_alloc(MemoryPool* pool, size_t size) {
    if (!pool || size == 0 || size > POOL_SIZE) return NULL;
    if (pool->protection_guard != MAGIC_NUMBER) {
        fprintf(stderr, "Memory pool corruption detected!\n");
        return NULL;
    }

    // Auf Mindestblockgröße aufrunden
    size = (size + MIN_BLOCK_SIZE - 1) & ~(MIN_BLOCK_SIZE - 1);

    pthread_mutex_lock(&pool->mutex);

    BlockHeader* current = pool->free_list;
    BlockHeader* previous = NULL;

    // First-fit Strategie mit Fragmentierungsvermeidung
    while (current) {
        if (!validate_block(current)) {
            fprintf(stderr, "Block corruption detected!\n");
            pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }

        if (!current->used && current->size >= size) {
            // Prüfen ob Block geteilt werden soll
            if (current->size >= size + sizeof(BlockHeader) + MIN_BLOCK_SIZE) {
                BlockHeader* new_block = (BlockHeader*)((uint8_t*)current + sizeof(BlockHeader) + size);
                new_block->size = current->size - size - sizeof(BlockHeader);
                new_block->used = false;
                new_block->magic = MAGIC_NUMBER;
                new_block->next = current->next;
                new_block->checksum = calculate_checksum(new_block);

                current->size = size;
                current->next = new_block;
            }

            current->used = true;
            current->checksum = calculate_checksum(current);
            pool->allocated_blocks++;

            pthread_mutex_unlock(&pool->mutex);
            return (void*)((uint8_t*)current + sizeof(BlockHeader));
        }

        previous = current;
        current = current->next;
    }

    pthread_mutex_unlock(&pool->mutex);
    return NULL;
}

// Memory Deallocation
void pool_free(MemoryPool* pool, void* ptr) {
    if (!pool || !ptr) return;
    if (pool->protection_guard != MAGIC_NUMBER) {
        fprintf(stderr, "Memory pool corruption detected!\n");
        return;
    }

    pthread_mutex_lock(&pool->mutex);

    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    
    // Validierung
    if (!validate_block(block)) {
        fprintf(stderr, "Block corruption detected during free!\n");
        pthread_mutex_unlock(&pool->mutex);
        return;
    }

    if (!block->used) {
        fprintf(stderr, "Double free detected!\n");
        pthread_mutex_unlock(&pool->mutex);
        return;
    }

    // Block als frei markieren
    block->used = false;
    pool->allocated_blocks--;

    // Benachbarte freie Blöcke zusammenführen
    BlockHeader* current = pool->free_list;
    while (current) {
        if (!current->used && current->next && !current->next->used) {
            current->size += sizeof(BlockHeader) + current->next->size;
            current->next = current->next->next;
            current->checksum = calculate_checksum(current);
            continue;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&pool->mutex);
}

// Pool Destruction
void destroy_memory_pool(MemoryPool* pool) {
    if (!pool) return;
    
    if (pool->allocated_blocks > 0) {
        fprintf(stderr, "Warning: Memory leak detected! %zu blocks still allocated\n", 
                pool->allocated_blocks);
    }

    pthread_mutex_destroy(&pool->mutex);
    free(pool->memory);
    free(pool);
}

// Debugging-Funktion
void pool_debug_info(MemoryPool* pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->mutex);
    
    printf("\nMemory Pool Debug Information:\n");
    printf("Total Size: %zu bytes\n", pool->total_size);
    printf("Allocated Blocks: %zu\n", pool->allocated_blocks);
    
    size_t free_blocks = 0;
    size_t total_free = 0;
    BlockHeader* current = pool->free_list;
    
    while (current) {
        if (!current->used) {
            free_blocks++;
            total_free += current->size;
        }
        current = current->next;
    }
    
    printf("Free Blocks: %zu\n", free_blocks);
    printf("Total Free Memory: %zu bytes\n", total_free);
    printf("Memory Utilization: %.2f%%\n", 
           (float)(pool->total_size - total_free) / pool->total_size * 100);
    
    pthread_mutex_unlock(&pool->mutex);
}