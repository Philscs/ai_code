#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// MurmurHash3 Implementierung für String-Hashing
uint32_t murmur3_32(const char *key, size_t len, uint32_t seed) {
    uint32_t h1 = seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = len / 4;
    const uint32_t *blocks = (const uint32_t *)(key);

    for(int i = 0; i < nblocks; i++) {
        uint32_t k1 = blocks[i];
        
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> (32 - 15));
        k1 *= c2;
        
        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> (32 - 13));
        h1 = h1 * 5 + 0xe6546b64;
    }

    const uint8_t *tail = (const uint8_t *)(key + nblocks * 4);
    uint32_t k1 = 0;

    switch(len & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> (32 - 15));
            k1 *= c2;
            h1 ^= k1;
    }

    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

typedef struct {
    uint8_t *bits;
    size_t size;      // Größe in Bits
    size_t numHashes; // Anzahl der Hash-Funktionen
} BloomFilter;

// Bloom Filter Initialisierung mit gewünschter False Positive Rate
BloomFilter* bloomFilter_create(size_t expectedItems, double falsePositiveRate) {
    BloomFilter *bf = malloc(sizeof(BloomFilter));
    
    // Optimale Größe und Anzahl der Hash-Funktionen berechnen
    size_t size = (size_t)(-((double)expectedItems * log(falsePositiveRate)) / (log(2) * log(2)));
    size_t numHashes = (size_t)(((double)size / expectedItems) * log(2));
    
    bf->bits = calloc((size + 7) / 8, 1); // Aufrunden auf volle Bytes
    bf->size = size;
    bf->numHashes = numHashes;
    
    return bf;
}

// Element zum Bloom Filter hinzufügen
void bloomFilter_add(BloomFilter *bf, const char *item) {
    size_t len = strlen(item);
    
    for(size_t i = 0; i < bf->numHashes; i++) {
        uint32_t hash = murmur3_32(item, len, i);
        size_t index = hash % bf->size;
        bf->bits[index / 8] |= 1 << (index % 8);
    }
}

// Überprüfen ob ein Element möglicherweise im Filter ist
bool bloomFilter_mightContain(BloomFilter *bf, const char *item) {
    size_t len = strlen(item);
    
    for(size_t i = 0; i < bf->numHashes; i++) {
        uint32_t hash = murmur3_32(item, len, i);
        size_t index = hash % bf->size;
        if(!(bf->bits[index / 8] & (1 << (index % 8)))) {
            return false;
        }
    }
    return true;
}

// Bloom Filter freigeben
void bloomFilter_free(BloomFilter *bf) {
    free(bf->bits);
    free(bf);
}

// Cache-Struktur die Bloom Filter verwendet
typedef struct {
    BloomFilter *filter;
    char **keys;
    void **values;
    size_t capacity;
    size_t size;
} BloomCache;

// Cache mit Bloom Filter erstellen
BloomCache* bloomCache_create(size_t capacity, double falsePositiveRate) {
    BloomCache *cache = malloc(sizeof(BloomCache));
    cache->filter = bloomFilter_create(capacity, falsePositiveRate);
    cache->keys = malloc(capacity * sizeof(char*));
    cache->values = malloc(capacity * sizeof(void*));
    cache->capacity = capacity;
    cache->size = 0;
    return cache;
}

// Element zum Cache hinzufügen
void bloomCache_put(BloomCache *cache, const char *key, void *value) {
    if(cache->size >= cache->capacity) {
        // Einfache Verdrängungsstrategie: Ältestes Element entfernen
        free(cache->keys[0]);
        memmove(cache->keys, cache->keys + 1, (cache->size - 1) * sizeof(char*));
        memmove(cache->values, cache->values + 1, (cache->size - 1) * sizeof(void*));
        cache->size--;
    }
    
    bloomFilter_add(cache->filter, key);
    cache->keys[cache->size] = strdup(key);
    cache->values[cache->size] = value;
    cache->size++;
}

// Element aus dem Cache abrufen
void* bloomCache_get(BloomCache *cache, const char *key) {
    // Erst Bloom Filter prüfen
    if(!bloomFilter_mightContain(cache->filter, key)) {
        return NULL; // Definitiv nicht im Cache
    }
    
    // Linear durch die Keys suchen
    for(size_t i = 0; i < cache->size; i++) {
        if(strcmp(cache->keys[i], key) == 0) {
            return cache->values[i];
        }
    }
    
    return NULL; // False positive vom Bloom Filter
}

// Cache freigeben
void bloomCache_free(BloomCache *cache) {
    bloomFilter_free(cache->filter);
    for(size_t i = 0; i < cache->size; i++) {
        free(cache->keys[i]);
    }
    free(cache->keys);
    free(cache->values);
    free(cache);
}

// Beispiel zur Verwendung
int main() {
    // Cache mit 1000 Elementen und 1% False Positive Rate erstellen
    BloomCache *cache = bloomCache_create(1000, 0.01);
    
    // Einige Testwerte
    int value1 = 42;
    bloomCache_put(cache, "key1", &value1);
    
    char *value2 = "Hello World";
    bloomCache_put(cache, "key2", value2);
    
    // Werte abrufen
    int *result1 = bloomCache_get(cache, "key1");
    if(result1) {
        printf("key1: %d\n", *result1);
    }
    
    char *result2 = bloomCache_get(cache, "key2");
    if(result2) {
        printf("key2: %s\n", result2);
    }
    
    // Nicht existierender Key
    void *result3 = bloomCache_get(cache, "key3");
    if(!result3) {
        printf("key3 nicht gefunden\n");
    }
    
    bloomCache_free(cache);
    return 0;
}