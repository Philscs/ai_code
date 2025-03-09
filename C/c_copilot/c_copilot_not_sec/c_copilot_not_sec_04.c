#include <pthread.h>
#include <stdlib.h>

struct Node {
    void* data;
    size_t size;
    struct Node* next;
};

struct MemoryPool {
    struct Node* head;
    pthread_mutex_t lock;
};

struct MemoryPool* create_memory_pool() {
    struct MemoryPool* pool = (struct MemoryPool*)malloc(sizeof(struct MemoryPool));
    if (pool != NULL) {
        pool->head = NULL;
        pthread_mutex_init(&pool->lock, NULL);
    }
    return pool;
}

void destroy_memory_pool(struct MemoryPool* pool) {
    if (pool != NULL) {
        pthread_mutex_destroy(&pool->lock);
        free(pool);
    }
}

void* custom_malloc(struct MemoryPool* pool, size_t size) {
    void* mem = NULL;
    if (pool != NULL) {
        pthread_mutex_lock(&pool->lock);
        struct Node* node = pool->head;
        if (node != NULL) {
            pool->head = node->next;
            mem = node->data;
            free(node);
        }
        pthread_mutex_unlock(&pool->lock);
    }
    if (mem == NULL) {
        mem = malloc(size);
    }
    return mem;
}

void custom_free(struct MemoryPool* pool, void* mem) {
    if (pool != NULL && mem != NULL) {
        pthread_mutex_lock(&pool->lock);
        struct Node* node = (struct Node*)malloc(sizeof(struct Node));
        if (node != NULL) {
            node->data = mem;
            node->size = 0;
            node->next = pool->head;
            pool->head = node;
        }
        pthread_mutex_unlock(&pool->lock);
    }
}
