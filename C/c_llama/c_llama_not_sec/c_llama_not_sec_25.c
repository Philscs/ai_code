#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// Maximum number of threads
#define MAX_THREADS 10

// Maximum size of the hash table
#define MAX_SIZE 1024

// Structure for a single entry in the hash table
typedef struct Entry {
    int key;
    void* value;
    struct Entry** buckets;
} Entry;

// Structure for the concurrent hash table
typedef struct ConcurrentHashTable {
    Entry** buckets;
    size_t size;
    size_t used;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ConcurrentHashTable;

// Function to insert a new key-value pair into the hash table
void* insertThread(void* arg) {
    ConcurrentHashTable* ht = (ConcurrentHashTable*)arg;
    int key, value;
    // Simulate insertion of a key-value pair
    key = 12345;
    value = "Hello World";
    pthread_mutex_lock(&ht->mutex);
    size_t index = ht->hash_(key) % ht->size_;
    Entry* entry = ht->buckets[index];

    if (entry == nullptr) {
        // New bucket, allocate and add the new entry
        entry = malloc(sizeof(Entry));
        entry->buckets = malloc(ht->size * sizeof Entry**);
        for (int i = 0; i < ht->size; i++) {
            entry->buckets[i] = nullptr;
        }
    } else {
        // Update existing entry with the same key
        for (int i = 0; i < ht->size; i++) {
            if ((Entry*)ht->buckets[index]->buckets[i]->key == key) {
                (ht->buckets[index]->buckets[i])->value = &value;
                return NULL;
            }
        }

        // Add new entry to the bucket's list of entries
        Entry* newEntry = malloc(sizeof(Entry));
        newEntry->key = key;
        newEntry->value = &value;
        Entry** bucketsList = (Entry**)malloc(ht->size * sizeof(void*));
        for (int i = 0; i < ht->size; i++) {
            bucketsList[i] = (Entry*)ht->buckets[index]->buckets[i];
        }
        ht->buckets[index]->buckets = bucketsList;
    }

    ht->used++;
    size_t new_size = ht->size * 2;
    if (ht->used > new_size) {
        resize(ht, new_size);
    }
    pthread_cond_signal(&ht->cond);
    pthread_mutex_unlock(&ht->mutex);
    return NULL;
}

// Function to retrieve a value by key from the hash table
void* getThread(void* arg) {
    ConcurrentHashTable* ht = (ConcurrentHashTable*)arg;
    int key, value;
    // Simulate retrieval of a key-value pair
    key = 12345;
    pthread_mutex_lock(&ht->mutex);
    Entry* entry = ht->buckets[ht->hash_(key) % ht->size];
    if (entry == nullptr || entry->buckets == nullptr) {
        pthread_cond_wait(&ht->cond, &ht->mutex);
    }

    value = (char*)((Entry*)entry)->value;
    for (int i = 0; i < ht->size; i++) {
        Entry* bucketEntry = (Entry*)ht->buckets[ht->hash_(key) % ht->size]->buckets[i];
        if ((BucketEntry*)bucketEntry != nullptr && ((BucketEntry*)bucketEntry)->key == key) {
            value = (char*)((BucketEntry*)bucketEntry)->value;
            break;
        }
    }

    pthread_mutex_unlock(&ht->mutex);
    return (void*)value;
}

// Function to resize the hash table
void resize(ConcurrentHashTable* ht, size_t new_size) {
    // Free old buckets
    for (int i = 0; i < ht->size; i++) {
        free(ht->buckets[i]);
    }
    // Allocate new buckets
    ht->buckets = malloc(new_size * sizeof(Entry**));
    for (int i = 0; i < new_size; i++) {
        ht->buckets[i] = nullptr;
    }

    // Recreate the buckets with new sizes
}

// Hash function to map keys to indices
size_t hash_(const int key) {
    return key % MAX_SIZE;
}

int main() {
    ConcurrentHashTable* ht = (ConcurrentHashTable*)malloc(sizeof(ConcurrentHashTable));
    pthread_mutex_init(&ht->mutex, NULL);
    pthread_cond_init(&ht->cond, NULL);

    // Initialize the hash table
    for (int i = 0; i < MAX_SIZE; i++) {
        ht->buckets[i] = nullptr;
    }

    // Create and start threads
    pthread_t threads[MAX_THREADS];
    ConcurrentHashTable* threads_[] = {ht};

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads_[i], NULL, insertThread, threads_);
    }
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Wait for the insertion thread to finish
    ConcurrentHashTable* threads_2[] = {ht};
    for (int i = 0; i < MAX_THREADS / 2; i++) {
        pthread_create(&threads_[i * 2], NULL, getThread, threads_);
    }
    for (int i = 0; i < MAX_THREADS / 2; i++) {
        pthread_join(threads[i * 2], NULL);
    }

    return 0;
}
