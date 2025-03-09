#include <stdlib.h>
#include <pthread.h>

struct Entry {
    char* key;
    void* value;
    struct Entry* next;
};

struct HashTable {
    struct Entry** buckets;
    size_t size;
    size_t used;
    pthread_mutex_t* locks;
};

void hash_table_init(struct HashTable* table, size_t initial_size) {
    table->buckets = (struct Entry**)calloc(initial_size, sizeof(struct Entry*));
    table->size = initial_size;
    table->used = 0;
    table->locks = (pthread_mutex_t*)calloc(initial_size, sizeof(pthread_mutex_t));
    for (size_t i = 0; i < initial_size; i++) {
        pthread_mutex_init(&table->locks[i], NULL);
    }
}

void hash_table_resize(struct HashTable* table, size_t new_size) {
    struct Entry** new_buckets = (struct Entry**)calloc(new_size, sizeof(struct Entry*));
    pthread_mutex_t* new_locks = (pthread_mutex_t*)calloc(new_size, sizeof(pthread_mutex_t));
    for (size_t i = 0; i < new_size; i++) {
        pthread_mutex_init(&new_locks[i], NULL);
    }

    for (size_t i = 0; i < table->size; i++) {
        pthread_mutex_lock(&table->locks[i]);
        struct Entry* entry = table->buckets[i];
        while (entry != NULL) {
            struct Entry* next = entry->next;
            size_t new_bucket = hash(entry->key) % new_size;
            entry->next = new_buckets[new_bucket];
            new_buckets[new_bucket] = entry;
            entry = next;
        }
        pthread_mutex_unlock(&table->locks[i]);
    }

    free(table->buckets);
    free(table->locks);
    table->buckets = new_buckets;
    table->locks = new_locks;
    table->size = new_size;
}

void hash_table_insert(struct HashTable* table, char* key, void* value) {
    size_t bucket = hash(key) % table->size;
    pthread_mutex_lock(&table->locks[bucket]);
    struct Entry* entry = table->buckets[bucket];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            pthread_mutex_unlock(&table->locks[bucket]);
            return;
        }
        entry = entry->next;
    }
    entry = (struct Entry*)malloc(sizeof(struct Entry));
    entry->key = key;
    entry->value = value;
    entry->next = table->buckets[bucket];
    table->buckets[bucket] = entry;
    table->used++;
    pthread_mutex_unlock(&table->locks[bucket]);

    if (table->used > table->size * 0.75) {
        hash_table_resize(table, table->size * 2);
    }
}

void* hash_table_get(struct HashTable* table, char* key) {
    size_t bucket = hash(key) % table->size;
    pthread_mutex_lock(&table->locks[bucket]);
    struct Entry* entry = table->buckets[bucket];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            pthread_mutex_unlock(&table->locks[bucket]);
            return entry->value;
        }
        entry = entry->next;
    }
    pthread_mutex_unlock(&table->locks[bucket]);
    return NULL;
}

void hash_table_destroy(struct HashTable* table) {
    for (size_t i = 0; i < table->size; i++) {
        pthread_mutex_destroy(&table->locks[i]);
    }
    free(table->buckets);
    free(table->locks);
}
