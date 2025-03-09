#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

// Constants
#define INITIAL_CAPACITY 16
#define LOAD_FACTOR 0.75

// Struct for Hash Table Entries
typedef struct Entry {
    char *key;
    void *value;
    struct Entry *next;
} Entry;

// Struct for Hash Table
typedef struct HashTable {
    Entry **buckets;
    size_t capacity;
    size_t size;
    pthread_mutex_t lock;
} HashTable;

// FNV-1a hash function (secure and simple)
uint32_t hash_function(const char *key) {
    uint32_t hash = 2166136261u;
    while (*key) {
        hash ^= (uint8_t)*key++;
        hash *= 16777619;
    }
    return hash;
}

// Create a new hash table
HashTable *create_table(size_t capacity) {
    HashTable *table = malloc(sizeof(HashTable));
    if (!table) {
        perror("Failed to allocate hash table");
        exit(EXIT_FAILURE);
    }
    table->buckets = calloc(capacity, sizeof(Entry *));
    if (!table->buckets) {
        perror("Failed to allocate buckets");
        free(table);
        exit(EXIT_FAILURE);
    }
    table->capacity = capacity;
    table->size = 0;
    pthread_mutex_init(&table->lock, NULL);
    return table;
}

// Free memory used by the hash table
void free_table(HashTable *table) {
    pthread_mutex_lock(&table->lock);
    for (size_t i = 0; i < table->capacity; i++) {
        Entry *entry = table->buckets[i];
        while (entry) {
            Entry *temp = entry;
            entry = entry->next;
            free(temp->key);
            free(temp);
        }
    }
    free(table->buckets);
    pthread_mutex_unlock(&table->lock);
    pthread_mutex_destroy(&table->lock);
    free(table);
}

// Resize the hash table
void resize_table(HashTable *table) {
    size_t new_capacity = table->capacity * 2;
    Entry **new_buckets = calloc(new_capacity, sizeof(Entry *));
    if (!new_buckets) {
        perror("Failed to allocate new buckets");
        return;
    }

    // Rehash entries
    for (size_t i = 0; i < table->capacity; i++) {
        Entry *entry = table->buckets[i];
        while (entry) {
            Entry *next = entry->next;
            uint32_t index = hash_function(entry->key) % new_capacity;
            entry->next = new_buckets[index];
            new_buckets[index] = entry;
            entry = next;
        }
    }

    free(table->buckets);
    table->buckets = new_buckets;
    table->capacity = new_capacity;
}

// Insert into the hash table
void insert(HashTable *table, const char *key, void *value) {
    pthread_mutex_lock(&table->lock);

    if ((double)table->size / table->capacity > LOAD_FACTOR) {
        resize_table(table);
    }

    uint32_t index = hash_function(key) % table->capacity;
    Entry *entry = table->buckets[index];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            pthread_mutex_unlock(&table->lock);
            return;
        }
        entry = entry->next;
    }

    // Create new entry
    Entry *new_entry = malloc(sizeof(Entry));
    if (!new_entry) {
        perror("Failed to allocate entry");
        pthread_mutex_unlock(&table->lock);
        return;
    }
    new_entry->key = strdup(key);
    if (!new_entry->key) {
        perror("Failed to allocate key");
        free(new_entry);
        pthread_mutex_unlock(&table->lock);
        return;
    }
    new_entry->value = value;
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
    table->size++;

    pthread_mutex_unlock(&table->lock);
}

// Retrieve from the hash table
void *get(HashTable *table, const char *key) {
    pthread_mutex_lock(&table->lock);
    uint32_t index = hash_function(key) % table->capacity;
    Entry *entry = table->buckets[index];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            pthread_mutex_unlock(&table->lock);
            return entry->value;
        }
        entry = entry->next;
    }

    pthread_mutex_unlock(&table->lock);
    return NULL;
}

// Iterator structure
typedef struct Iterator {
    HashTable *table;
    size_t bucket_index;
    Entry *current;
} Iterator;

// Initialize an iterator
Iterator create_iterator(HashTable *table) {
    Iterator it = {table, 0, NULL};
    return it;
}

// Get the next element using the iterator
Entry *next(Iterator *it) {
    while (!it->current && it->bucket_index < it->table->capacity) {
        it->current = it->table->buckets[it->bucket_index++];
    }

    if (it->current) {
        Entry *entry = it->current;
        it->current = it->current->next;
        return entry;
    }

    return NULL;
}

int main() {
    HashTable *table = create_table(INITIAL_CAPACITY);

    insert(table, "key1", "value1");
    insert(table, "key2", "value2");

    printf("Key1: %s\n", (char *)get(table, "key1"));
    printf("Key2: %s\n", (char *)get(table, "key2"));

    Iterator it = create_iterator(table);
    Entry *entry;
    while ((entry = next(&it))) {
        printf("%s: %s\n", entry->key, (char *)entry->value);
    }

    free_table(table);
    return 0;
}
