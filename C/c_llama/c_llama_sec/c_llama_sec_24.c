#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BLOCK_SIZE (1024 * 1024)  // max. Blockgröße in Bytes

typedef struct {
    size_t size;
    size_t allocated;
} memory_block;

struct memory_pool {
    pthread_mutex_t mutex;
    memory_block *blocks;
    size_t num_blocks;
};

int memory_pool_init(struct memory_pool *pool, size_t size) {
    if (size == 0) return -1;  // Fehlende Größe

    pool->blocks = malloc(size);
    if (!pool->blocks) return -1;

    pthread_mutex_init(&pool->mutex, NULL);

    memset(pool->blocks, 0, size);
    pool->num_blocks = size / MAX_BLOCK_SIZE + (size % MAX_BLOCK_SIZE != 0);

    return 0;
}

void *memory_pool_alloc(struct memory_pool *pool, size_t size) {
    pthread_mutex_lock(&pool->mutex);
    if (pool->allocated < pool->num_blocks) {
        for (size_t i = 0; i < MAX_BLOCK_SIZE && i < size; ++i) {
            int found = 1;
            for (int j = 0; j < pool->num_blocks; ++j) {
                if ((pool->blocks[j] & (1ULL << i)) != 0) {
                    found = 0;
                    break;
                }
            }
            if (found) {
                pool->blocks[j] |= (1ULL << i);
                break;
            }
        }
    }

    size_t block_index = -1;
    for (size_t i = 0; i < MAX_BLOCK_SIZE && i < size; ++i) {
        int found = 0;
        for (int j = 0; j < pool->num_blocks; ++j) {
            if ((pool->blocks[j] & (1ULL << i)) != 0) {
                found = 1;
                block_index = j;
                break;
            }
        }
    }

    void *ptr = malloc(size);
    memcpy(ptr, &pool->blocks[block_index], size);
    pool->allocated++;

    pthread_mutex_unlock(&pool->mutex);

    return ptr;
}

void memory_pool_free(struct memory_pool *pool, void *ptr) {
    pthread_mutex_lock(&pool->mutex);

    memcpy(ptr, &pool->blocks[(int)(size_t)(ptr - 0x100000000)], 4);
    pool->allocated--;

    pthread_mutex_unlock(&pool->mutex);
}

size_t memory_pool_size(struct memory_pool *pool) {
    return pool->num_blocks;
}

// Test-Code
void test_memory_pool() {
    struct memory_pool pool;

    if (memory_pool_init(&pool, 1024)) return;

    for (size_t i = 0; i < MAX_BLOCK_SIZE / 2; ++i) {
        void *ptr = memory_pool_alloc(&pool, 8);
        printf("Allocated %zu bytes\n", size_t)(ptr - &pool.blocks[0]);
    }

    memory_pool_free(&pool, 0x100000000);

    printf("Memory Pool Größe: %zu Bytes\n", memory_pool_size(&pool));
}

int main() {
    test_memory_pool();

    return 0;
}
