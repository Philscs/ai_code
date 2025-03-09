#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Strukturdefinitionen
struct Entry {
    char* key;
    void* value;
    struct Entry* next;
};

struct HashTable {
    struct Entry** buckets;
    size_t size;
    size_t used;
    pthread_mutex_t lock;
};

// Hash-Funktion
size_t hash(const char* key, size_t size) {
    size_t hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % size;
}

// Hilfsfunktion zum Erstellen eines neuen Entries
struct Entry* create_entry(const char* key, void* value) {
    struct Entry* entry = malloc(sizeof(struct Entry));
    entry->key = strdup(key);
    entry->value = value;
    entry->next = NULL;
    return entry;
}

// HashTable initialisieren
struct HashTable* create_hash_table(size_t size) {
    struct HashTable* table = malloc(sizeof(struct HashTable));
    table->buckets = calloc(size, sizeof(struct Entry*));
    table->size = size;
    table->used = 0;
    pthread_mutex_init(&table->lock, NULL);
    return table;
}

// Automatisches Resizing
void resize_table(struct HashTable* table) {
    size_t new_size = table->size * 2;
    struct Entry** new_buckets = calloc(new_size, sizeof(struct Entry*));

    for (size_t i = 0; i < table->size; i++) {
        struct Entry* entry = table->buckets[i];
        while (entry) {
            struct Entry* next = entry->next;
            size_t new_index = hash(entry->key, new_size);
            entry->next = new_buckets[new_index];
            new_buckets[new_index] = entry;
            entry = next;
        }
    }

    free(table->buckets);
    table->buckets = new_buckets;
    table->size = new_size;
}

// Eintrag hinzufügen
void hash_table_put(struct HashTable* table, const char* key, void* value) {
    pthread_mutex_lock(&table->lock);

    if (table->used >= table->size * 0.75) {
        resize_table(table);
    }

    size_t index = hash(key, table->size);
    struct Entry* entry = table->buckets[index];
    struct Entry* prev = NULL;

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            pthread_mutex_unlock(&table->lock);
            return;
        }
        prev = entry;
        entry = entry->next;
    }

    struct Entry* new_entry = create_entry(key, value);
    if (prev) {
        prev->next = new_entry;
    } else {
        table->buckets[index] = new_entry;
    }

    table->used++;
    pthread_mutex_unlock(&table->lock);
}

// Wert abrufen
void* hash_table_get(struct HashTable* table, const char* key) {
    pthread_mutex_lock(&table->lock);

    size_t index = hash(key, table->size);
    struct Entry* entry = table->buckets[index];

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

// Eintrag entfernen
void hash_table_remove(struct HashTable* table, const char* key) {
    pthread_mutex_lock(&table->lock);

    size_t index = hash(key, table->size);
    struct Entry* entry = table->buckets[index];
    struct Entry* prev = NULL;

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                table->buckets[index] = entry->next;
            }
            free(entry->key);
            free(entry);
            table->used--;
            pthread_mutex_unlock(&table->lock);
            return;
        }
        prev = entry;
        entry = entry->next;
    }

    pthread_mutex_unlock(&table->lock);
}

// Freigeben der HashTable
void free_hash_table(struct HashTable* table) {
    for (size_t i = 0; i < table->size; i++) {
        struct Entry* entry = table->buckets[i];
        while (entry) {
            struct Entry* next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    free(table->buckets);
    pthread_mutex_destroy(&table->lock);
    free(table);
}

// Beispiel zur Nutzung
int main() {
    struct HashTable* table = create_hash_table(8);

    hash_table_put(table, "key1", "value1");
    hash_table_put(table, "key2", "value2");

    printf("key1: %s\n", (char*)hash_table_get(table, "key1"));
    printf("key2: %s\n", (char*)hash_table_get(table, "key2"));

    hash_table_remove(table, "key1");
    printf("key1 after removal: %s\n", (char*)hash_table_get(table, "key1"));

    free_hash_table(table);
    return 0;
}
