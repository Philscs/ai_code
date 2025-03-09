#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define CACHE_SIZE 4 // Anzahl der Cache-Slots
#define MEMORY_SIZE 16 // Simulierte Hauptspeichergröße

typedef struct {
    int tag;
    bool valid;
    bool dirty;
    int last_used;
    bool clock_bit;
} CacheLine;

CacheLine cache[CACHE_SIZE];
int memory[MEMORY_SIZE];
int clock_hand = 0;

// Initialisiere Cache und Speicher
void init_cache() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache[i].tag = -1;
        cache[i].valid = false;
        cache[i].dirty = false;
        cache[i].last_used = 0;
        cache[i].clock_bit = false;
    }

    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = i * 10; // Beispiel-Daten
    }
}

// Funktion zur Simulation von Write-Through
void write_through(int index, int value) {
    int memory_index = cache[index].tag;
    memory[memory_index] = value;
}

// Funktion zur Simulation von Write-Back
void write_back(int index) {
    if (cache[index].dirty) {
        int memory_index = cache[index].tag;
        memory[memory_index] = cache[index].tag; // Zurückschreiben
        cache[index].dirty = false;
    }
}

// Suche nach Cache-Eintrag
int find_cache_index(int tag) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].valid && cache[i].tag == tag) {
            return i;
        }
    }
    return -1;
}

// LRU Algorithmus
int lru_replacement() {
    int oldest = 0;
    for (int i = 1; i < CACHE_SIZE; i++) {
        if (cache[i].last_used < cache[oldest].last_used) {
            oldest = i;
        }
    }
    return oldest;
}

// FIFO Algorithmus
int fifo_replacement() {
    static int fifo_index = 0;
    int index = fifo_index;
    fifo_index = (fifo_index + 1) % CACHE_SIZE;
    return index;
}

// Clock Algorithmus
int clock_replacement() {
    while (true) {
        if (!cache[clock_hand].clock_bit) {
            int replacement = clock_hand;
            clock_hand = (clock_hand + 1) % CACHE_SIZE;
            return replacement;
        } else {
            cache[clock_hand].clock_bit = false;
            clock_hand = (clock_hand + 1) % CACHE_SIZE;
        }
    }
}

// Laden von Daten in den Cache
void load_to_cache(int tag, int algorithm) {
    int index = -1;
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (!cache[i].valid) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        if (algorithm == 0) {
            index = lru_replacement();
        } else if (algorithm == 1) {
            index = fifo_replacement();
        } else if (algorithm == 2) {
            index = clock_replacement();
        }

        write_back(index);
    }

    cache[index].tag = tag;
    cache[index].valid = true;
    cache[index].dirty = false;
    cache[index].last_used = clock();
    cache[index].clock_bit = true;
}

// Leseoperation
int read(int address, int algorithm) {
    int tag = address;
    int index = find_cache_index(tag);

    if (index == -1) {
        printf("Cache-Miss! Laden von Adresse %d\n", address);
        load_to_cache(tag, algorithm);
        index = find_cache_index(tag);
    } else {
        printf("Cache-Hit!\n");
    }

    cache[index].last_used = clock();
    return memory[tag];
}

// Schreiboperation
void write(int address, int value, int algorithm, bool write_through_mode) {
    int tag = address;
    int index = find_cache_index(tag);

    if (index == -1) {
        printf("Cache-Miss! Laden von Adresse %d\n", address);
        load_to_cache(tag, algorithm);
        index = find_cache_index(tag);
    } else {
        printf("Cache-Hit!\n");
    }

    if (write_through_mode) {
        write_through(index, value);
    } else {
        cache[index].dirty = true;
    }

    memory[tag] = value;
}

int main() {
    init_cache();
    printf("Lese Adresse 5: %d\n", read(5, 0));
    write(5, 100, 0, true);
    printf("Schreibe Adresse 5 mit Wert 100 im Write-Through-Modus\n");
    printf("Lese Adresse 5: %d\n", read(5, 0));
    write(6, 200, 1, false);
    printf("Schreibe Adresse 6 mit Wert 200 im Write-Back-Modus\n");
    printf("Lese Adresse 6: %d\n", read(6, 2));

    return 0;
}
