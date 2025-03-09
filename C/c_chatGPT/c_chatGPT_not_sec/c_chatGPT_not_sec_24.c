#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define CACHE_SIZE 100  // Größe des Caches

// Struktur für den Bloom-Filter
typedef struct {
    unsigned char *bit_array;
    size_t bit_array_size;
    size_t num_hash_functions;
} BloomFilter;

// Hilfsfunktionen für Hashes
unsigned int hash1(const char *key) {
    unsigned int hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

unsigned int hash2(const char *key) {
    unsigned int hash = 0;
    int c;
    while ((c = *key++))
        hash = c + (hash << 6) + (hash << 16) - hash;
    return hash;
}

// Initialisierung des Bloom-Filters
BloomFilter *bloom_filter_init(size_t bit_array_size, size_t num_hash_functions) {
    BloomFilter *bf = malloc(sizeof(BloomFilter));
    bf->bit_array_size = bit_array_size;
    bf->num_hash_functions = num_hash_functions;
    bf->bit_array = calloc(bit_array_size, sizeof(unsigned char));
    return bf;
}

// Einfügen eines Schlüssels in den Bloom-Filter
void bloom_filter_add(BloomFilter *bf, const char *key) {
    unsigned int hash_values[2] = {hash1(key), hash2(key)};
    for (size_t i = 0; i < bf->num_hash_functions; i++) {
        size_t index = (hash_values[0] + i * hash_values[1]) % bf->bit_array_size;
        bf->bit_array[index] = 1;
    }
}

// Überprüfung, ob ein Schlüssel im Bloom-Filter enthalten ist
bool bloom_filter_contains(BloomFilter *bf, const char *key) {
    unsigned int hash_values[2] = {hash1(key), hash2(key)};
    for (size_t i = 0; i < bf->num_hash_functions; i++) {
        size_t index = (hash_values[0] + i * hash_values[1]) % bf->bit_array_size;
        if (bf->bit_array[index] == 0)
            return false;
    }
    return true;
}

// Freigeben des Bloom-Filters
void bloom_filter_free(BloomFilter *bf) {
    free(bf->bit_array);
    free(bf);
}

// Cache-System
typedef struct {
    char *data[CACHE_SIZE];
    size_t size;
    BloomFilter *bloom_filter;
} Cache;

// Initialisierung des Caches
Cache *cache_init(double false_positive_rate) {
    Cache *cache = malloc(sizeof(Cache));
    cache->size = 0;

    // Berechnung der optimalen Bloom-Filter-Parameter
    size_t bit_array_size = -(CACHE_SIZE * log(false_positive_rate)) / (log(2) * log(2));
    size_t num_hash_functions = (bit_array_size / CACHE_SIZE) * log(2);

    cache->bloom_filter = bloom_filter_init(bit_array_size, num_hash_functions);
    return cache;
}

// Hinzufügen von Daten zum Cache
void cache_add(Cache *cache, const char *key) {
    if (cache->size < CACHE_SIZE) {
        cache->data[cache->size++] = strdup(key);
        bloom_filter_add(cache->bloom_filter, key);
    } else {
        printf("Cache ist voll!\n");
    }
}

// Überprüfung, ob Daten im Cache sind
bool cache_contains(Cache *cache, const char *key) {
    if (bloom_filter_contains(cache->bloom_filter, key)) {
        for (size_t i = 0; i < cache->size; i++) {
            if (strcmp(cache->data[i], key) == 0)
                return true;
        }
        printf("False Positive: %s\n", key);
    }
    return false;
}

// Freigeben des Caches
void cache_free(Cache *cache) {
    for (size_t i = 0; i < cache->size; i++) {
        free(cache->data[i]);
    }
    bloom_filter_free(cache->bloom_filter);
    free(cache);
}

int main() {
    double false_positive_rate = 0.01; // 1% False-Positive-Rate
    Cache *cache = cache_init(false_positive_rate);

    cache_add(cache, "apple");
    cache_add(cache, "banana");
    cache_add(cache, "cherry");

    printf("apple in cache: %s\n", cache_contains(cache, "apple") ? "Ja" : "Nein");
    printf("grape in cache: %s\n", cache_contains(cache, "grape") ? "Ja" : "Nein");

    cache_free(cache);
    return 0;
}
