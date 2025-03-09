#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

// Error codes
#define POOL_SUCCESS 0
#define POOL_ERROR_INIT -1
#define POOL_ERROR_FULL -2
#define POOL_ERROR_EMPTY -3
#define POOL_ERROR_INVALID -4

// Object structure that will be pooled
typedef struct {
    int id;
    void* data;
    bool in_use;
} PoolObject;

// Object pool structure
typedef struct {
    PoolObject* objects;
    int capacity;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    bool (*initialize_object)(PoolObject*);
    void (*cleanup_object)(PoolObject*);
} ObjectPool;

// Initialize the object pool
int pool_init(ObjectPool* pool, int capacity,
             bool (*init_fn)(PoolObject*),
             void (*cleanup_fn)(PoolObject*)) {
    if (capacity <= 0 || !pool) {
        return POOL_ERROR_INVALID;
    }

    // Initialize pool attributes
    pool->capacity = capacity;
    pool->count = 0;
    pool->initialize_object = init_fn;
    pool->cleanup_object = cleanup_fn;

    // Allocate memory for objects
    pool->objects = (PoolObject*)calloc(capacity, sizeof(PoolObject));
    if (!pool->objects) {
        return POOL_ERROR_INIT;
    }

    // Initialize synchronization primitives
    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        free(pool->objects);
        return POOL_ERROR_INIT;
    }
    if (pthread_cond_init(&pool->not_full, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        free(pool->objects);
        return POOL_ERROR_INIT;
    }
    if (pthread_cond_init(&pool->not_empty, NULL) != 0) {
        pthread_cond_destroy(&pool->not_full);
        pthread_mutex_destroy(&pool->lock);
        free(pool->objects);
        return POOL_ERROR_INIT;
    }

    // Initialize all objects in the pool
    for (int i = 0; i < capacity; i++) {
        pool->objects[i].id = i;
        pool->objects[i].in_use = false;
        if (pool->initialize_object && !pool->initialize_object(&pool->objects[i])) {
            // Cleanup previously initialized objects
            for (int j = 0; j < i; j++) {
                if (pool->cleanup_object) {
                    pool->cleanup_object(&pool->objects[j]);
                }
            }
            pthread_cond_destroy(&pool->not_empty);
            pthread_cond_destroy(&pool->not_full);
            pthread_mutex_destroy(&pool->lock);
            free(pool->objects);
            return POOL_ERROR_INIT;
        }
    }

    return POOL_SUCCESS;
}

// Acquire an object from the pool
PoolObject* pool_acquire(ObjectPool* pool) {
    if (!pool) {
        return NULL;
    }

    pthread_mutex_lock(&pool->lock);

    // Wait while the pool is full
    while (pool->count >= pool->capacity) {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }

    // Find an available object
    PoolObject* object = NULL;
    for (int i = 0; i < pool->capacity; i++) {
        if (!pool->objects[i].in_use) {
            object = &pool->objects[i];
            object->in_use = true;
            pool->count++;
            break;
        }
    }

    pthread_mutex_unlock(&pool->lock);
    pthread_cond_signal(&pool->not_empty);

    return object;
}

// Release an object back to the pool
int pool_release(ObjectPool* pool, PoolObject* object) {
    if (!pool || !object) {
        return POOL_ERROR_INVALID;
    }

    pthread_mutex_lock(&pool->lock);

    if (!object->in_use || object->id < 0 || object->id >= pool->capacity) {
        pthread_mutex_unlock(&pool->lock);
        return POOL_ERROR_INVALID;
    }

    object->in_use = false;
    pool->count--;

    pthread_mutex_unlock(&pool->lock);
    pthread_cond_signal(&pool->not_full);

    return POOL_SUCCESS;
}

// Destroy the object pool and cleanup resources
void pool_destroy(ObjectPool* pool) {
    if (!pool) {
        return;
    }

    pthread_mutex_lock(&pool->lock);

    // Cleanup all objects
    for (int i = 0; i < pool->capacity; i++) {
        if (pool->cleanup_object) {
            pool->cleanup_object(&pool->objects[i]);
        }
    }

    // Free memory and destroy synchronization primitives
    free(pool->objects);
    pthread_mutex_unlock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->not_full);
    pthread_cond_destroy(&pool->not_empty);
}

// Example usage functions
bool example_init_object(PoolObject* obj) {
    obj->data = malloc(sizeof(int));
    return obj->data != NULL;
}

void example_cleanup_object(PoolObject* obj) {
    free(obj->data);
    obj->data = NULL;
}

// Example usage
int main() {
    ObjectPool pool;
    int result = pool_init(&pool, 5, example_init_object, example_cleanup_object);
    if (result != POOL_SUCCESS) {
        fprintf(stderr, "Failed to initialize pool: %d\n", result);
        return 1;
    }

    // Example usage in multiple threads would go here
    PoolObject* obj = pool_acquire(&pool);
    if (obj) {
        // Use the object
        pool_release(&pool, obj);
    }

    pool_destroy(&pool);
    return 0;
}