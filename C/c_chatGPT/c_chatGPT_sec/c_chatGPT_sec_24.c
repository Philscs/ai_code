#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#define POOL_SIZE 1024 // Gesamtgröße des Pools in Bytes
#define BLOCK_SIZE 32  // Größe eines Blocks in Bytes
#define NUM_BLOCKS (POOL_SIZE / BLOCK_SIZE)

typedef struct {
    uint8_t memory[POOL_SIZE];       // Speicherbereich des Pools
    uint8_t block_status[NUM_BLOCKS]; // Status jedes Blocks (0: frei, 1: belegt)
    pthread_mutex_t lock;           // Mutex für Thread-Sicherheit
} MemoryPool;

MemoryPool pool;

// Initialisiert den Memory Pool
void init_memory_pool() {
    memset(pool.memory, 0, POOL_SIZE);
    memset(pool.block_status, 0, NUM_BLOCKS);
    pthread_mutex_init(&pool.lock, NULL);
}

// Allokiert einen Block deterministisch
void* allocate_block() {
    pthread_mutex_lock(&pool.lock);
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (pool.block_status[i] == 0) { // Block ist frei
            pool.block_status[i] = 1;   // Block als belegt markieren
            pthread_mutex_unlock(&pool.lock);
            return &pool.memory[i * BLOCK_SIZE];
        }
    }
    pthread_mutex_unlock(&pool.lock);
    return NULL; // Kein freier Block
}

// Gibt einen Block frei
void free_block(void* ptr) {
    if (ptr < (void*)pool.memory || ptr >= (void*)(pool.memory + POOL_SIZE)) {
        fprintf(stderr, "Ungültige Speicheradresse!\n");
        return;
    }

    int index = ((uint8_t*)ptr - pool.memory) / BLOCK_SIZE;

    pthread_mutex_lock(&pool.lock);
    if (pool.block_status[index] == 1) {
        pool.block_status[index] = 0; // Block als frei markieren
    } else {
        fprintf(stderr, "Block ist bereits frei!\n");
    }
    pthread_mutex_unlock(&pool.lock);
}

// Führt eine Fehlerüberprüfung durch
void check_pool_integrity() {
    pthread_mutex_lock(&pool.lock);
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (pool.block_status[i] != 0 && pool.block_status[i] != 1) {
            fprintf(stderr, "Fehlerhafte Blockstatus-Information bei Block %d!\n", i);
        }
    }
    pthread_mutex_unlock(&pool.lock);
}

// Testprogramm
int main() {
    init_memory_pool();

    void* block1 = allocate_block();
    if (block1) {
        printf("Block 1 allokiert bei Adresse %p\n", block1);
    }

    void* block2 = allocate_block();
    if (block2) {
        printf("Block 2 allokiert bei Adresse %p\n", block2);
    }

    free_block(block1);
    printf("Block 1 freigegeben\n");

    free_block(block2);
    printf("Block 2 freigegeben\n");

    check_pool_integrity();

    return 0;
}
