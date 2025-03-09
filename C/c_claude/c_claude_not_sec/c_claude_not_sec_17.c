#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define CACHE_SIZE 4
#define MEMORY_SIZE 16

typedef enum {
    LRU,
    FIFO,
    CLOCK
} ReplacementPolicy;

typedef enum {
    WRITE_THROUGH,
    WRITE_BACK
} WritePolicy;

typedef struct {
    int tag;
    int data;
    bool valid;
    bool dirty;  // Für Write-Back
    bool referenced;  // Für Clock-Algorithmus
} CacheLine;

typedef struct {
    CacheLine lines[CACHE_SIZE];
    int memory[MEMORY_SIZE];
    ReplacementPolicy replacementPolicy;
    WritePolicy writePolicy;
    int fifoPointer;  // Für FIFO
    int clockPointer;  // Für Clock
    int* lruCounter;  // Für LRU
} Cache;

// Cache initialisieren
Cache* initCache(ReplacementPolicy rp, WritePolicy wp) {
    Cache* cache = (Cache*)malloc(sizeof(Cache));
    cache->replacementPolicy = rp;
    cache->writePolicy = wp;
    cache->fifoPointer = 0;
    cache->clockPointer = 0;
    cache->lruCounter = (int*)malloc(CACHE_SIZE * sizeof(int));
    
    // Cache-Zeilen initialisieren
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache->lines[i].valid = false;
        cache->lines[i].dirty = false;
        cache->lines[i].referenced = false;
        cache->lines[i].tag = -1;
        cache->lines[i].data = 0;
        cache->lruCounter[i] = 0;
    }
    
    // Hauptspeicher initialisieren
    for (int i = 0; i < MEMORY_SIZE; i++) {
        cache->memory[i] = i * 10;  // Beispieldaten
    }
    
    return cache;
}

// LRU-Zähler aktualisieren
void updateLRUCounter(Cache* cache, int accessedIndex) {
    int currentCount = cache->lruCounter[accessedIndex];
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache->lruCounter[i] < currentCount) {
            cache->lruCounter[i]++;
        }
    }
    cache->lruCounter[accessedIndex] = 0;
}

// Freien Cache-Platz finden oder Opfer wählen
int findVictim(Cache* cache) {
    // Erst nach freiem Platz suchen
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (!cache->lines[i].valid) return i;
    }
    
    // Wenn kein freier Platz, dann nach Ersetzungsstrategie wählen
    switch (cache->replacementPolicy) {
        case LRU: {
            int maxCounter = -1;
            int victim = 0;
            for (int i = 0; i < CACHE_SIZE; i++) {
                if (cache->lruCounter[i] > maxCounter) {
                    maxCounter = cache->lruCounter[i];
                    victim = i;
                }
            }
            return victim;
        }
        
        case FIFO: {
            int victim = cache->fifoPointer;
            cache->fifoPointer = (cache->fifoPointer + 1) % CACHE_SIZE;
            return victim;
        }
        
        case CLOCK: {
            while (true) {
                if (!cache->lines[cache->clockPointer].referenced) {
                    int victim = cache->clockPointer;
                    cache->clockPointer = (cache->clockPointer + 1) % CACHE_SIZE;
                    return victim;
                }
                cache->lines[cache->clockPointer].referenced = false;
                cache->clockPointer = (cache->clockPointer + 1) % CACHE_SIZE;
            }
        }
    }
    return 0;
}

// Write-Back: Dirty Line in Hauptspeicher zurückschreiben
void writeBack(Cache* cache, int index) {
    if (cache->lines[index].dirty) {
        cache->memory[cache->lines[index].tag] = cache->lines[index].data;
        cache->lines[index].dirty = false;
    }
}

// Cache-Zugriff für Lesen
int readCache(Cache* cache, int address) {
    // Cache durchsuchen
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache->lines[i].valid && cache->lines[i].tag == address) {
            // Cache Hit
            cache->lines[i].referenced = true;
            if (cache->replacementPolicy == LRU) {
                updateLRUCounter(cache, i);
            }
            return cache->lines[i].data;
        }
    }
    
    // Cache Miss
    int victim = findVictim(cache);
    
    // Write-Back wenn nötig
    if (cache->writePolicy == WRITE_BACK) {
        writeBack(cache, victim);
    }
    
    // Neue Daten laden
    cache->lines[victim].valid = true;
    cache->lines[victim].tag = address;
    cache->lines[victim].data = cache->memory[address];
    cache->lines[victim].referenced = true;
    cache->lines[victim].dirty = false;
    
    if (cache->replacementPolicy == LRU) {
        updateLRUCounter(cache, victim);
    }
    
    return cache->lines[victim].data;
}

// Cache-Zugriff für Schreiben
void writeCache(Cache* cache, int address, int data) {
    // Cache durchsuchen
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache->lines[i].valid && cache->lines[i].tag == address) {
            // Cache Hit
            cache->lines[i].data = data;
            cache->lines[i].referenced = true;
            
            if (cache->writePolicy == WRITE_THROUGH) {
                cache->memory[address] = data;
            } else {  // WRITE_BACK
                cache->lines[i].dirty = true;
            }
            
            if (cache->replacementPolicy == LRU) {
                updateLRUCounter(cache, i);
            }
            return;
        }
    }
    
    // Cache Miss
    int victim = findVictim(cache);
    
    // Write-Back wenn nötig
    if (cache->writePolicy == WRITE_BACK) {
        writeBack(cache, victim);
    }
    
    // Neue Daten schreiben
    cache->lines[victim].valid = true;
    cache->lines[victim].tag = address;
    cache->lines[victim].data = data;
    cache->lines[victim].referenced = true;
    
    if (cache->writePolicy == WRITE_THROUGH) {
        cache->memory[address] = data;
        cache->lines[victim].dirty = false;
    } else {  // WRITE_BACK
        cache->lines[victim].dirty = true;
    }
    
    if (cache->replacementPolicy == LRU) {
        updateLRUCounter(cache, victim);
    }
}

// Cache-Status ausgeben
void printCacheStatus(Cache* cache) {
    printf("\nCache Status:\n");
    printf("Index\tValid\tDirty\tRef\tTag\tData\n");
    for (int i = 0; i < CACHE_SIZE; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n",
               i,
               cache->lines[i].valid,
               cache->lines[i].dirty,
               cache->lines[i].referenced,
               cache->lines[i].tag,
               cache->lines[i].data);
    }
    printf("\n");
}

// Speicher freigeben
void freeCache(Cache* cache) {
    free(cache->lruCounter);
    free(cache);
}

// Beispiel zur Verwendung
int main() {
    // Cache mit LRU und Write-Back erstellen
    Cache* cache = initCache(LRU, WRITE_BACK);
    
    // Einige Operationen durchführen
    printf("Reading address 5: %d\n", readCache(cache, 5));
    printf("Reading address 7: %d\n", readCache(cache, 7));
    writeCache(cache, 5, 100);
    printf("Reading address 5 after write: %d\n", readCache(cache, 5));
    
    printCacheStatus(cache);
    
    // Speicher freigeben
    freeCache(cache);
    
    return 0;
}