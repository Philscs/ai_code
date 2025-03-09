#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define CACHE_SIZE 1000
#define HASH_FUNCTIONS 3

typedef struct {
    bool* filter;
    int filter_size;
    int* hash_seeds;
    int false_positive_rate;
} BloomFilter;

BloomFilter* createBloomFilter(int size, int false_positive_rate) {
    BloomFilter* bloomFilter = (BloomFilter*)malloc(sizeof(BloomFilter));
    bloomFilter->filter_size = size;
    bloomFilter->false_positive_rate = false_positive_rate;

    int num_bits = ceil((size * log(1.0 / false_positive_rate)) / log(2));
    bloomFilter->filter = (bool*)calloc(num_bits, sizeof(bool));

    bloomFilter->hash_seeds = (int*)malloc(HASH_FUNCTIONS * sizeof(int));
    for (int i = 0; i < HASH_FUNCTIONS; i++) {
        bloomFilter->hash_seeds[i] = rand();
    }

    return bloomFilter;
}

void destroyBloomFilter(BloomFilter* bloomFilter) {
    free(bloomFilter->filter);
    free(bloomFilter->hash_seeds);
    free(bloomFilter);
}

int hash(int seed, const char* key) {
    int hash = seed;
    int len = strlen(key);
    for (int i = 0; i < len; i++) {
        hash = (hash * 31) + key[i];
    }
    return hash;
}

void insert(BloomFilter* bloomFilter, const char* key) {
    for (int i = 0; i < HASH_FUNCTIONS; i++) {
        int index = hash(bloomFilter->hash_seeds[i], key) % bloomFilter->filter_size;
        bloomFilter->filter[index] = true;
    }
}

bool contains(BloomFilter* bloomFilter, const char* key) {
    for (int i = 0; i < HASH_FUNCTIONS; i++) {
        int index = hash(bloomFilter->hash_seeds[i], key) % bloomFilter->filter_size;
        if (!bloomFilter->filter[index]) {
            return false;
        }
    }
    return true;
}

int main() {
    BloomFilter* bloomFilter = createBloomFilter(CACHE_SIZE, 0.01);

    insert(bloomFilter, "key1");
    insert(bloomFilter, "key2");
    insert(bloomFilter, "key3");

    printf("Contains key1: %s\n", contains(bloomFilter, "key1") ? "true" : "false");
    printf("Contains key4: %s\n", contains(bloomFilter, "key4") ? "true" : "false");

    destroyBloomFilter(bloomFilter);

    return 0;
}
