#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

// Sichere Hash-Funktion (SHA-256 ähnlich)
#define ROTRIGHT(word,bits) (((word) >> (bits)) | ((word) << (32-(bits))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

// Initial Bucket Size und Load Factor
#define INITIAL_SIZE 16
#define LOAD_FACTOR_THRESHOLD 0.75

typedef struct Node {
    char* key;
    void* value;
    struct Node* next;
} Node;

typedef struct {
    Node** buckets;
    size_t size;
    size_t count;
    pthread_mutex_t lock;
    pthread_mutex_t* bucket_locks;
    bool is_iterating;
} HashTable;

typedef struct {
    HashTable* table;
    size_t current_bucket;
    Node* current_node;
    pthread_mutex_t iterator_lock;
} Iterator;

// Sichere Hash-Funktion
uint32_t secure_hash(const char* key) {
    uint32_t h = 0x811C9DC5;
    unsigned char* str = (unsigned char*)key;
    
    while (*str) {
        // Überprüfung auf Integer Overflow
        uint32_t next_h;
        if (__builtin_mul_overflow(h, 0x01000193, &next_h)) {
            // Fallback bei Overflow
            h = (h >> 16) ^ *str;
        } else {
            h = next_h ^ *str;
        }
        str++;
    }
    
    // Zusätzliche Sicherheitstransformationen
    h = ROTRIGHT(h, 13);
    h = CH(h, ROTRIGHT(h, 7), ROTRIGHT(h, 11));
    
    return h;
}

// Initialisierung der Hash Table
HashTable* hash_table_create() {
    HashTable* table = (HashTable*)calloc(1, sizeof(HashTable));
    if (!table) return NULL;
    
    table->size = INITIAL_SIZE;
    table->count = 0;
    table->is_iterating = false;
    
    // Allokierung der Buckets
    table->buckets = (Node**)calloc(table->size, sizeof(Node*));
    if (!table->buckets) {
        free(table);
        return NULL;
    }
    
    // Initialisierung der Locks
    if (pthread_mutex_init(&table->lock, NULL) != 0) {
        free(table->buckets);
        free(table);
        return NULL;
    }
    
    // Initialisierung der Bucket-Locks
    table->bucket_locks = (pthread_mutex_t*)calloc(table->size, sizeof(pthread_mutex_t));
    if (!table->bucket_locks) {
        pthread_mutex_destroy(&table->lock);
        free(table->buckets);
        free(table);
        return NULL;
    }
    
    for (size_t i = 0; i < table->size; i++) {
        if (pthread_mutex_init(&table->bucket_locks[i], NULL) != 0) {
            // Cleanup bei Fehler
            for (size_t j = 0; j < i; j++) {
                pthread_mutex_destroy(&table->bucket_locks[j]);
            }
            pthread_mutex_destroy(&table->lock);
            free(table->bucket_locks);
            free(table->buckets);
            free(table);
            return NULL;
        }
    }
    
    return table;
}

// Sichere Resizing-Funktion
bool resize_table(HashTable* table) {
    if (!table || table->is_iterating) return false;
    
    pthread_mutex_lock(&table->lock);
    
    // Überprüfung auf Integer Overflow bei der neuen Größe
    size_t new_size;
    if (__builtin_mul_overflow(table->size, 2, &new_size)) {
        pthread_mutex_unlock(&table->lock);
        return false;
    }
    
    Node** new_buckets = (Node**)calloc(new_size, sizeof(Node*));
    pthread_mutex_t* new_locks = (pthread_mutex_t*)calloc(new_size, sizeof(pthread_mutex_t));
    
    if (!new_buckets || !new_locks) {
        free(new_buckets);
        free(new_locks);
        pthread_mutex_unlock(&table->lock);
        return false;
    }
    
    // Initialisierung der neuen Locks
    for (size_t i = 0; i < new_size; i++) {
        if (pthread_mutex_init(&new_locks[i], NULL) != 0) {
            for (size_t j = 0; j < i; j++) {
                pthread_mutex_destroy(&new_locks[j]);
            }
            free(new_locks);
            free(new_buckets);
            pthread_mutex_unlock(&table->lock);
            return false;
        }
    }
    
    // Rehashing der existierenden Einträge
    for (size_t i = 0; i < table->size; i++) {
        Node* current = table->buckets[i];
        while (current) {
            Node* next = current->next;
            uint32_t hash = secure_hash(current->key);
            size_t new_index = hash % new_size;
            
            current->next = new_buckets[new_index];
            new_buckets[new_index] = current;
            
            current = next;
        }
    }
    
    // Cleanup der alten Locks
    for (size_t i = 0; i < table->size; i++) {
        pthread_mutex_destroy(&table->bucket_locks[i]);
    }
    
    free(table->bucket_locks);
    free(table->buckets);
    
    table->buckets = new_buckets;
    table->bucket_locks = new_locks;
    table->size = new_size;
    
    pthread_mutex_unlock(&table->lock);
    return true;
}

// Thread-sicheres Einfügen
bool hash_table_put(HashTable* table, const char* key, void* value) {
    if (!table || !key) return false;
    
    pthread_mutex_lock(&table->lock);
    
    // Überprüfung des Load Factors
    float load_factor = (float)table->count / table->size;
    if (load_factor >= LOAD_FACTOR_THRESHOLD) {
        if (!resize_table(table)) {
            pthread_mutex_unlock(&table->lock);
            return false;
        }
    }
    
    uint32_t hash = secure_hash(key);
    size_t index = hash % table->size;
    
    pthread_mutex_lock(&table->bucket_locks[index]);
    pthread_mutex_unlock(&table->lock);
    
    // Suche nach existierendem Key
    Node* current = table->buckets[index];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            current->value = value;
            pthread_mutex_unlock(&table->bucket_locks[index]);
            return true;
        }
        current = current->next;
    }
    
    // Neuen Node erstellen
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        pthread_mutex_unlock(&table->bucket_locks[index]);
        return false;
    }
    
    new_node->key = strdup(key);
    if (!new_node->key) {
        free(new_node);
        pthread_mutex_unlock(&table->bucket_locks[index]);
        return false;
    }
    
    new_node->value = value;
    new_node->next = table->buckets[index];
    table->buckets[index] = new_node;
    
    __atomic_add_fetch(&table->count, 1, __ATOMIC_SEQ_CST);
    
    pthread_mutex_unlock(&table->bucket_locks[index]);
    return true;
}

// Thread-sicheres Abrufen
void* hash_table_get(HashTable* table, const char* key) {
    if (!table || !key) return NULL;
    
    uint32_t hash = secure_hash(key);
    size_t index = hash % table->size;
    
    pthread_mutex_lock(&table->bucket_locks[index]);
    
    Node* current = table->buckets[index];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            void* value = current->value;
            pthread_mutex_unlock(&table->bucket_locks[index]);
            return value;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&table->bucket_locks[index]);
    return NULL;
}

// Iterator-Funktionen
Iterator* iterator_create(HashTable* table) {
    if (!table) return NULL;
    
    pthread_mutex_lock(&table->lock);
    
    if (table->is_iterating) {
        pthread_mutex_unlock(&table->lock);
        return NULL;
    }
    
    Iterator* iterator = (Iterator*)malloc(sizeof(Iterator));
    if (!iterator) {
        pthread_mutex_unlock(&table->lock);
        return NULL;
    }
    
    if (pthread_mutex_init(&iterator->iterator_lock, NULL) != 0) {
        free(iterator);
        pthread_mutex_unlock(&table->lock);
        return NULL;
    }
    
    iterator->table = table;
    iterator->current_bucket = 0;
    iterator->current_node = NULL;
    table->is_iterating = true;
    
    pthread_mutex_unlock(&table->lock);
    return iterator;
}

bool iterator_has_next(Iterator* iterator) {
    if (!iterator || !iterator->table) return false;
    
    pthread_mutex_lock(&iterator->iterator_lock);
    
    if (iterator->current_node && iterator->current_node->next) {
        pthread_mutex_unlock(&iterator->iterator_lock);
        return true;
    }
    
    size_t bucket = iterator->current_node ? 
                    iterator->current_bucket + 1 : iterator->current_bucket;
                    
    while (bucket < iterator->table->size) {
        if (iterator->table->buckets[bucket]) {
            pthread_mutex_unlock(&iterator->iterator_lock);
            return true;
        }
        bucket++;
    }
    
    pthread_mutex_unlock(&iterator->iterator_lock);
    return false;
}

Node* iterator_next(Iterator* iterator) {
    if (!iterator || !iterator->table) return NULL;
    
    pthread_mutex_lock(&iterator->iterator_lock);
    
    if (iterator->current_node && iterator->current_node->next) {
        iterator->current_node = iterator->current_node->next;
        Node* result = iterator->current_node;
        pthread_mutex_unlock(&iterator->iterator_lock);
        return result;
    }
    
    size_t bucket = iterator->current_node ? 
                    iterator->current_bucket + 1 : iterator->current_bucket;
                    
    while (bucket < iterator->table->size) {
        if (iterator->table->buckets[bucket]) {
            iterator->current_bucket = bucket;
            iterator->current_node = iterator->table->buckets[bucket];
            Node* result = iterator->current_node;
            pthread_mutex_unlock(&iterator->iterator_lock);
            return result;
        }
        bucket++;
    }
    
    pthread_mutex_unlock(&iterator->iterator_lock);
    return NULL;
}

void iterator_destroy(Iterator* iterator) {
    if (!iterator || !iterator->table) return;
    
    pthread_mutex_lock(&iterator->table->lock);
    iterator->table->is_iterating = false;
    pthread_mutex_unlock(&iterator->table->lock);
    
    pthread_mutex_destroy(&iterator->iterator_lock);
    free(iterator);
}

// Aufräumen der Hash Table
void hash_table_destroy(HashTable* table) {
    if (!table) return;
    
    pthread_mutex_lock(&table->lock);
    
    for (size_t i = 0; i < table->size; i++) {
        pthread_mutex_lock(&table->bucket_locks[i]);
        Node* current = table->buckets[i];
        while (current) {
            Node* next = current->next;
            free(current->key);
            free(current);
            current = next;
        }
        pthread_mutex_unlock(&table->bucket_locks[i]);
        pthread_mutex_destroy(&table->bucket_locks[i]);
    }
    
    free(table->bucket_locks);
    free(table->buckets);
    pthread_mutex_unlock(&table->lock);
    pthread_mutex_destroy(&table->lock);
    free(table);
}