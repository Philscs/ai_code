#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define PAGE_SIZE 4096
#define NUM_PAGES 256
#define NUM_FRAMES 128
#define TLB_SIZE 16

// Strukturen
typedef struct {
    int page_number;
    int frame_number;
    bool valid;
    bool dirty;
} PageTableEntry;

typedef struct {
    int page_number;
    int frame_number;
    int last_used;
} TLBEntry;

// Globale Variablen
PageTableEntry page_table[NUM_PAGES];
TLBEntry tlb[TLB_SIZE];
char physical_memory[NUM_FRAMES * PAGE_SIZE];
int clock_hand = 0;
int time_counter = 0;

// Initialisierung
void init_memory_system() {
    // Page Table initialisieren
    for (int i = 0; i < NUM_PAGES; i++) {
        page_table[i].valid = false;
        page_table[i].dirty = false;
        page_table[i].frame_number = -1;
    }
    
    // TLB initialisieren
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page_number = -1;
        tlb[i].frame_number = -1;
        tlb[i].last_used = -1;
    }
    
    // Physischen Speicher initialisieren
    memset(physical_memory, 0, NUM_FRAMES * PAGE_SIZE);
}

// TLB-Lookup
int search_tlb(int page_number) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page_number == page_number) {
            tlb[i].last_used = time_counter++;
            return tlb[i].frame_number;
        }
    }
    return -1;
}

// TLB-Update
void update_tlb(int page_number, int frame_number) {
    int lru_index = 0;
    int min_time = tlb[0].last_used;
    
    // LRU-Eintrag finden
    for (int i = 1; i < TLB_SIZE; i++) {
        if (tlb[i].last_used < min_time) {
            min_time = tlb[i].last_used;
            lru_index = i;
        }
    }
    
    // TLB aktualisieren
    tlb[lru_index].page_number = page_number;
    tlb[lru_index].frame_number = frame_number;
    tlb[lru_index].last_used = time_counter++;
}

// Clock Algorithm für Page Replacement
int clock_algorithm() {
    while (true) {
        if (!page_table[clock_hand].valid || !page_table[clock_hand].dirty) {
            int victim_frame = page_table[clock_hand].frame_number;
            page_table[clock_hand].valid = false;
            clock_hand = (clock_hand + 1) % NUM_PAGES;
            return victim_frame;
        }
        page_table[clock_hand].dirty = false;
        clock_hand = (clock_hand + 1) % NUM_PAGES;
    }
}

// Seitenersetzung
int page_replacement() {
    return clock_algorithm();
}

// Hauptfunktion für Speicherzugriffe
void* access_memory(int logical_address) {
    int page_number = (logical_address / PAGE_SIZE);
    int offset = logical_address % PAGE_SIZE;
    
    // TLB-Lookup
    int frame_number = search_tlb(page_number);
    
    if (frame_number == -1) {  // TLB Miss
        if (!page_table[page_number].valid) {  // Page Fault
            frame_number = page_replacement();
            // Hier würde normalerweise die Seite von der Festplatte geladen
            page_table[page_number].frame_number = frame_number;
            page_table[page_number].valid = true;
            page_table[page_number].dirty = false;
        } else {
            frame_number = page_table[page_number].frame_number;
        }
        update_tlb(page_number, frame_number);
    }
    
    return &physical_memory[frame_number * PAGE_SIZE + offset];
}

// Beispiel für die Verwendung
int main() {
    init_memory_system();
    
    // Beispiel für Speicherzugriffe
    int test_address = 12345;
    void* physical_addr = access_memory(test_address);
    
    printf("Logische Adresse: %d\n", test_address);
    printf("Physische Adresse: %p\n", physical_addr);
    
    return 0;
}