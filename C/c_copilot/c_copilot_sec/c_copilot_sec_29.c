#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_OBJECTS 10

typedef struct {
    // Object data
    int value;
    // Additional fields as needed
} Object;

typedef struct {
    Object* objects[MAX_OBJECTS];
    int count;
    pthread_mutex_t lock;
    pthread_cond_t available;
} ObjectPool;

ObjectPool* createObjectPool() {
    ObjectPool* pool = (ObjectPool*)malloc(sizeof(ObjectPool));
    pool->count = 0;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->available, NULL);
    return pool;
}

void destroyObjectPool(ObjectPool* pool) {
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->available);
    free(pool);
}

Object* getObject(ObjectPool* pool) {
    pthread_mutex_lock(&pool->lock);
    while (pool->count == 0) {
        pthread_cond_wait(&pool->available, &pool->lock);
    }
    Object* object = pool->objects[--pool->count];
    pthread_mutex_unlock(&pool->lock);
    return object;
}

void returnObject(ObjectPool* pool, Object* object) {
    pthread_mutex_lock(&pool->lock);
    if (pool->count < MAX_OBJECTS) {
        pool->objects[pool->count++] = object;
        pthread_cond_signal(&pool->available);
    } else {
        // Handle error: pool is full
        printf("Object pool is full. Cannot return object.\n");
    }
    pthread_mutex_unlock(&pool->lock);
}

void* worker(void* arg) {
    ObjectPool* pool = (ObjectPool*)arg;
    Object* object = getObject(pool);
    // Use the object
    printf("Worker thread: Got object with value %d\n", object->value);
    // Return the object to the pool
    returnObject(pool, object);
    return NULL;
}

int main() {
    ObjectPool* pool = createObjectPool();

    // Create worker threads
    pthread_t threads[5];
    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, worker, pool);
    }

    // Wait for worker threads to finish
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    destroyObjectPool(pool);
    return 0;
}
