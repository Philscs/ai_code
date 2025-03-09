#include <stdio.h>
#include <stdlib.h>

#define CACHE_SIZE 4

typedef struct {
    int valid;
    int tag;
    int dirty;
    int lastUsed;
} CacheLine;

typedef struct {
    CacheLine lines[CACHE_SIZE];
} Cache;

void initCache(Cache* cache) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache->lines[i].valid = 0;
        cache->lines[i].tag = -1;
        cache->lines[i].dirty = 0;
        cache->lines[i].lastUsed = 0;
    }
}

int findLRU(Cache* cache) {
    int minIndex = 0;
    int minLastUsed = cache->lines[0].lastUsed;

    for (int i = 1; i < CACHE_SIZE; i++) {
        if (cache->lines[i].lastUsed < minLastUsed) {
            minIndex = i;
            minLastUsed = cache->lines[i].lastUsed;
        }
    }

    return minIndex;
}

int findFIFO(Cache* cache) {
    int minIndex = 0;
    int minTag = cache->lines[0].tag;

    for (int i = 1; i < CACHE_SIZE; i++) {
        if (cache->lines[i].tag < minTag) {
            minIndex = i;
            minTag = cache->lines[i].tag;
        }
    }

    return minIndex;
}

int findClock(Cache* cache, int* clockHand) {
    while (1) {
        if (!cache->lines[*clockHand].valid) {
            return *clockHand;
        }

        if (cache->lines[*clockHand].lastUsed == 0) {
            cache->lines[*clockHand].lastUsed = 1;
        } else {
            cache->lines[*clockHand].lastUsed = 0;
            (*clockHand)++;
            if (*clockHand >= CACHE_SIZE) {
                *clockHand = 0;
            }
        }
    }
}

void readCache(Cache* cache, int address, int algorithm) {
    int tag = address / CACHE_SIZE;
    int index = address % CACHE_SIZE;

    if (cache->lines[index].valid && cache->lines[index].tag == tag) {
        printf("Cache hit!\n");
        cache->lines[index].lastUsed = 1;
    } else {
        printf("Cache miss!\n");

        if (cache->lines[index].dirty) {
            printf("Write back to memory: %d\n", cache->lines[index].tag * CACHE_SIZE + index);
        }

        cache->lines[index].valid = 1;
        cache->lines[index].tag = tag;
        cache->lines[index].dirty = 0;
        cache->lines[index].lastUsed = 1;

        if (algorithm == 0) {
            int lruIndex = findLRU(cache);
            cache->lines[lruIndex].valid = 0;
        } else if (algorithm == 1) {
            int fifoIndex = findFIFO(cache);
            cache->lines[fifoIndex].valid = 0;
        } else if (algorithm == 2) {
            int clockHand = 0;
            int clockIndex = findClock(cache, &clockHand);
            cache->lines[clockIndex].valid = 0;
        }
    }
}

void writeCache(Cache* cache, int address, int algorithm, int writeStrategy) {
    int tag = address / CACHE_SIZE;
    int index = address % CACHE_SIZE;

    if (cache->lines[index].valid && cache->lines[index].tag == tag) {
        printf("Cache hit!\n");
        cache->lines[index].lastUsed = 1;

        if (writeStrategy == 1) {
            printf("Write through to memory: %d\n", address);
        } else if (writeStrategy == 2) {
            printf("Write back to cache: %d\n", address);
            cache->lines[index].dirty = 1;
        }
    } else {
        printf("Cache miss!\n");

        if (cache->lines[index].dirty) {
            printf("Write back to memory: %d\n", cache->lines[index].tag * CACHE_SIZE + index);
        }

        cache->lines[index].valid = 1;
        cache->lines[index].tag = tag;
        cache->lines[index].dirty = (writeStrategy == 2);
        cache->lines[index].lastUsed = 1;

        if (algorithm == 0) {
            int lruIndex = findLRU(cache);
            cache->lines[lruIndex].valid = 0;
        } else if (algorithm == 1) {
            int fifoIndex = findFIFO(cache);
            cache->lines[fifoIndex].valid = 0;
        } else if (algorithm == 2) {
            int clockHand = 0;
            int clockIndex = findClock(cache, &clockHand);
            cache->lines[clockIndex].valid = 0;
        }

        if (writeStrategy == 1) {
            printf("Write through to memory: %d\n", address);
        } else if (writeStrategy == 2) {
            printf("Write back to cache: %d\n", address);
            cache->lines[index].dirty = 1;
        }
    }
}

int main() {
    Cache cache;
    initCache(&cache);

    // Read example
    printf("Read example:\n");
    readCache(&cache, 5, 0); // LRU algorithm
    readCache(&cache, 6, 1); // FIFO algorithm
    readCache(&cache, 7, 2); // Clock algorithm

    // Write example
    printf("\nWrite example:\n");
    writeCache(&cache, 10, 0, 1); // LRU algorithm, Write-Through strategy
    writeCache(&cache, 11, 1, 2); // FIFO algorithm, Write-Back strategy
    writeCache(&cache, 12, 2, 1); // Clock algorithm, Write-Through strategy

    return 0;
}
