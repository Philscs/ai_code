#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

// Struktur für einen Node im Memory Pool
typedef struct {
    void* data;
    size_t size;
    struct Node* next;
} Node;

// Struktur für den Memory Pool
typedef struct {
    // Array von freeen Nodes
    Node** nodes;
    // Anzahl der frei verfügbaren Nodes
    int free_count;
    // Anzahl der ausgewerteten Nodes
    int garbage_collected;
    // Lock zum Synchronisierung von Zugriffen auf den Pool
    pthread_mutex_t lock;
} MemoryPool;

// Funktion zur Erstellung eines neuen Memory Pools
MemoryPool* memory_pool_create(size_t pool_size) {
    MemoryPool* pool = malloc(sizeof(MemoryPool));
    if (pool == NULL) return NULL;

    pool->nodes = calloc(pool_size, sizeof(Node*));
    if (pool->nodes == NULL) return NULL;

    for (int i = 0; i < pool_size; i++) {
        pool->nodes[i] = malloc(sizeof(Node));
        if (pool->nodes[i] == NULL) break;
        // Initalisierung des Nodes
        pool->nodes[i]->data = NULL;
        pool->nodes[i]->size = 0;
        pool->nodes[i]->next = NULL;
    }

    pool->free_count = pool_size;
    pool->garbage_collected = 0;

    pthread_mutex_init(&pool->lock, NULL);

    return pool;
}

// Funktion zum Auswerten von Nodes aus dem Pool
void memory_pool_garbage_collect(MemoryPool* pool) {
    pthread_mutex_lock(&pool->lock);
    // Durchsuchen des Pools nach ausgewerteten Nodes
    for (int i = 0; i < pool->free_count; i++) {
        if (pool->nodes[i]->data == NULL) {
            pool->nodes[i]->data = malloc(pool->nodes[i]->size);
            if (pool->nodes[i]->data == NULL) continue;
            // Initalisierung des ausgewerteten Data
            memcpy(pool->nodes[i]->data, NULL, pool->nodes[i]->size);
            pool->garbage_collected++;
        }
    }
    pthread_mutex_unlock(&pool->lock);
}

// Funktion zum Zuweisung eines Blocks von Daten aus dem Pool
void* memory_pool_allocate(MemoryPool* pool, size_t size) {
    pthread_mutex_lock(&pool->lock);
    // Suchen nach einem frei verfügbaren Block
    for (int i = 0; i < pool->free_count; i++) {
        if (pool->nodes[i]->size >= size) {
            void* data = malloc(pool->nodes[i]->data + size);
            if (data == NULL) continue;

            // Zuweisung des Blocks an den Pool
            pool->nodes[i]->data = data;
            pool->nodes[i]->size -= size;

            pthread_mutex_unlock(&pool->lock);
            return data;
        }
    }

    pthread_mutex_unlock(&pool->lock);

    // Block nicht gefunden, kein Data vorhanden
    return NULL;
}

// Funktion zum Freigeben eines Blocks von Daten im Pool
void memory_pool_free(MemoryPool* pool, void* data) {
    pthread_mutex_lock(&pool->lock);
    for (int i = 0; i < pool->free_count; i++) {
        if (pool->nodes[i]->data == data) {
            // Freigabe des Blocks
            pool->nodes[i]->size += data;
            break;
        }
    }
    pthread_mutex_unlock(&pool->lock);
}

// Funktion zum Auflösen eines Memory Pools
void memory_pool_destroy(MemoryPool* pool) {
    pthread_mutex_lock(&pool->lock);

    for (int i = 0; i < pool->free_count; i++) {
        free(pool->nodes[i]->data);
    }

    for (int i = 0; i < pool->free_count; i++) {
        free(pool->nodes[i]);
    }

    pthread_mutex_unlock(&pool->lock);

    pthread_mutex_destroy(&pool->lock);
}

// Beispiel usage
int main() {
    MemoryPool* pool = memory_pool_create(10);
    if (pool == NULL) return -1;

    void* data1 = memory_pool_allocate(pool, 5);
    void* data2 = memory_pool_allocate(pool, 3);

    printf("Data1: %p\n", data1);
    printf("Data2: %p\n", data2);

    memory_pool_free(pool, data1);
    memory_pool_free(pool, data2);

    return 0;
}
