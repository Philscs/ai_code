#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define INITIAL_SIZE 16
#define MAX_LOAD_FACTOR 0.75

typedef struct {
    char* key;
    int value;
} Entry;

typedef struct {
    Entry* entries;
    int size;
    int count;
    pthread_mutex_t lock;
} HashTable;

unsigned int hash(const char* key) {
    unsigned int hash = 5381;
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // djb2 hash function
    }

    return hash;
}

void initHashTable(HashTable* table) {
    table->entries = (Entry*)malloc(sizeof(Entry) * INITIAL_SIZE);
    table->size = INITIAL_SIZE;
    table->count = 0;
    pthread_mutex_init(&table->lock, NULL);
}

void destroyHashTable(HashTable* table) {
    free(table->entries);
    pthread_mutex_destroy(&table->lock);
}

void resizeHashTable(HashTable* table) {
    int newSize = table->size * 2;
    Entry* newEntries = (Entry*)malloc(sizeof(Entry) * newSize);
    memset(newEntries, 0, sizeof(Entry) * newSize);

    for (int i = 0; i < table->size; i++) {
        if (table->entries[i].key != NULL) {
            unsigned int index = hash(table->entries[i].key) % newSize;
            while (newEntries[index].key != NULL) {
                index = (index + 1) % newSize;
            }
            newEntries[index] = table->entries[i];
        }
    }

    free(table->entries);
    table->entries = newEntries;
    table->size = newSize;
}

void put(HashTable* table, const char* key, int value) {
    pthread_mutex_lock(&table->lock);

    if ((double)table->count / table->size > MAX_LOAD_FACTOR) {
        resizeHashTable(table);
    }

    unsigned int index = hash(key) % table->size;
    while (table->entries[index].key != NULL) {
        if (strcmp(table->entries[index].key, key) == 0) {
            table->entries[index].value = value;
            pthread_mutex_unlock(&table->lock);
            return;
        }
        index = (index + 1) % table->size;
    }

    table->entries[index].key = strdup(key);
    table->entries[index].value = value;
    table->count++;

    pthread_mutex_unlock(&table->lock);
}

int get(HashTable* table, const char* key) {
    pthread_mutex_lock(&table->lock);

    unsigned int index = hash(key) % table->size;
    while (table->entries[index].key != NULL) {
        if (strcmp(table->entries[index].key, key) == 0) {
            int value = table->entries[index].value;
            pthread_mutex_unlock(&table->lock);
            return value;
        }
        index = (index + 1) % table->size;
    }

    pthread_mutex_unlock(&table->lock);
    return -1;
}

int main() {
    HashTable table;
    initHashTable(&table);

    put(&table, "key1", 10);
    put(&table, "key2", 20);
    put(&table, "key3", 30);

    printf("Value for key1: %d\n", get(&table, "key1"));
    printf("Value for key2: %d\n", get(&table, "key2"));
    printf("Value for key3: %d\n", get(&table, "key3"));

    destroyHashTable(&table);

    return 0;
}
