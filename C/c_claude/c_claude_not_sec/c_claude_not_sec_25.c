#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define LOAD_FACTOR_THRESHOLD 0.75
#define INITIAL_SIZE 16
#define GROWTH_FACTOR 2

struct Entry {
    char* key;
    void* value;
    struct Entry* next;
    pthread_mutex_t lock;
};

struct HashTable {
    struct Entry** buckets;
    size_t size;
    size_t used;
    pthread_mutex_t global_lock;
    pthread_mutex_t resize_lock;
};

// Hash-Funktion
size_t hash_function(const char* key, size_t size) {
    size_t hash = 0;
    while (*key) {
        hash = (hash * 31 + *key) % size;
        key++;
    }
    return hash;
}

// Initialisierung der Hash Table
struct HashTable* hash_table_create() {
    struct HashTable* table = malloc(sizeof(struct HashTable));
    if (!table) return NULL;

    table->size = INITIAL_SIZE;
    table->used = 0;
    table->buckets = calloc(INITIAL_SIZE, sizeof(struct Entry*));
    
    if (!table->buckets) {
        free(table);
        return NULL;
    }

    pthread_mutex_init(&table->global_lock, NULL);
    pthread_mutex_init(&table->resize_lock, NULL);
    
    return table;
}

// Hilfsfunktion zum Erstellen eines neuen Eintrags
struct Entry* create_entry(const char* key, void* value) {
    struct Entry* entry = malloc(sizeof(struct Entry));
    if (!entry) return NULL;

    entry->key = strdup(key);
    entry->value = value;
    entry->next = NULL;
    pthread_mutex_init(&entry->lock, NULL);
    
    return entry;
}

// Resize-Operation
bool resize_table(struct HashTable* table) {
    pthread_mutex_lock(&table->resize_lock);
    
    // Überprüfe, ob ein anderer Thread bereits das Resizing durchgeführt hat
    if (((float)table->used / table->size) < LOAD_FACTOR_THRESHOLD) {
        pthread_mutex_unlock(&table->resize_lock);
        return true;
    }

    size_t new_size = table->size * GROWTH_FACTOR;
    struct Entry** new_buckets = calloc(new_size, sizeof(struct Entry*));
    if (!new_buckets) {
        pthread_mutex_unlock(&table->resize_lock);
        return false;
    }

    // Globaler Lock für die Reorganisation
    pthread_mutex_lock(&table->global_lock);

    // Rehashing aller Einträge
    for (size_t i = 0; i < table->size; i++) {
        struct Entry* current = table->buckets[i];
        while (current) {
            struct Entry* next = current->next;
            size_t new_index = hash_function(current->key, new_size);
            current->next = new_buckets[new_index];
            new_buckets[new_index] = current;
            current = next;
        }
    }

    free(table->buckets);
    table->buckets = new_buckets;
    table->size = new_size;

    pthread_mutex_unlock(&table->global_lock);
    pthread_mutex_unlock(&table->resize_lock);
    
    return true;
}

// Einfügen eines Elements
bool hash_table_put(struct HashTable* table, const char* key, void* value) {
    if (!table || !key) return false;

    // Überprüfe Load Factor und führe ggf. Resizing durch
    if (((float)table->used / table->size) >= LOAD_FACTOR_THRESHOLD) {
        if (!resize_table(table)) return false;
    }

    size_t index = hash_function(key, table->size);
    
    // Lock für den spezifischen Bucket
    pthread_mutex_lock(&table->global_lock);
    
    struct Entry* current = table->buckets[index];
    struct Entry* prev = NULL;

    // Suche nach existierendem Schlüssel
    while (current) {
        if (strcmp(current->key, key) == 0) {
            current->value = value;
            pthread_mutex_unlock(&table->global_lock);
            return true;
        }
        prev = current;
        current = current->next;
    }

    // Erstelle neuen Eintrag
    struct Entry* new_entry = create_entry(key, value);
    if (!new_entry) {
        pthread_mutex_unlock(&table->global_lock);
        return false;
    }

    if (prev) {
        prev->next = new_entry;
    } else {
        table->buckets[index] = new_entry;
    }

    table->used++;
    pthread_mutex_unlock(&table->global_lock);
    
    return true;
}

// Abrufen eines Elements
void* hash_table_get(struct HashTable* table, const char* key) {
    if (!table || !key) return NULL;

    size_t index = hash_function(key, table->size);
    
    pthread_mutex_lock(&table->global_lock);
    struct Entry* current = table->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            void* value = current->value;
            pthread_mutex_unlock(&table->global_lock);
            return value;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&table->global_lock);
    return NULL;
}

// Aufräumen der Hash Table
void hash_table_destroy(struct HashTable* table) {
    if (!table) return;

    for (size_t i = 0; i < table->size; i++) {
        struct Entry* current = table->buckets[i];
        while (current) {
            struct Entry* next = current->next;
            free(current->key);
            pthread_mutex_destroy(&current->lock);
            free(current);
            current = next;
        }
    }

    pthread_mutex_destroy(&table->global_lock);
    pthread_mutex_destroy(&table->resize_lock);
    free(table->buckets);
    free(table);
}