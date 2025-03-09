#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define POOL_SIZE 10

// Object struct
typedef struct {
    int id; // Example resource
} Object;

// Object Pool struct
typedef struct {
    Object* pool[POOL_SIZE];
    int available;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} ObjectPool;

// Initialize the object pool
ObjectPool* init_pool() {
    ObjectPool* pool = (ObjectPool*)malloc(sizeof(ObjectPool));
    if (!pool) {
        perror("Failed to allocate memory for the pool");
        return NULL;
    }

    pool->available = POOL_SIZE;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);

    for (int i = 0; i < POOL_SIZE; i++) {
        pool->pool[i] = (Object*)malloc(sizeof(Object));
        if (!pool->pool[i]) {
            perror("Failed to allocate memory for an object");
            pool->available = i;
            break;
        }
        pool->pool[i]->id = i;
    }

    return pool;
}

// Acquire an object from the pool
Object* acquire_object(ObjectPool* pool) {
    pthread_mutex_lock(&pool->lock);

    while (pool->available == 0) {
        pthread_cond_wait(&pool->cond, &pool->lock);
    }

    Object* obj = pool->pool[--pool->available];
    pthread_mutex_unlock(&pool->lock);

    return obj;
}

// Release an object back to the pool
void release_object(ObjectPool* pool, Object* obj) {
    if (!obj) return;

    pthread_mutex_lock(&pool->lock);

    if (pool->available < POOL_SIZE) {
        pool->pool[pool->available++] = obj;
        pthread_cond_signal(&pool->cond);
    } else {
        free(obj); // Cleanup if the pool is full
    }

    pthread_mutex_unlock(&pool->lock);
}

// Cleanup the entire pool
void cleanup_pool(ObjectPool* pool) {
    pthread_mutex_lock(&pool->lock);

    for (int i = 0; i < pool->available; i++) {
        free(pool->pool[i]);
    }

    pthread_mutex_unlock(&pool->lock);

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    free(pool);
}

// Example usage
int main() {
    ObjectPool* pool = init_pool();
    if (!pool) return EXIT_FAILURE;

    Object* obj1 = acquire_object(pool);
    printf("Acquired Object ID: %d\n", obj1->id);

    release_object(pool, obj1);
    printf("Released Object\n");

    cleanup_pool(pool);
    printf("Pool cleaned up\n");

    return 0;
}
