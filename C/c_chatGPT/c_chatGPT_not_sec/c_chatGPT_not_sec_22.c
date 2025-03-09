#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define FRAME_COUNT 4 // Anzahl der Frames im physischen Speicher
#define PAGE_COUNT 10 // Anzahl der Seiten

// Struktur einer Seite
typedef struct {
    int page_number;
    int last_used_time; // Für LRU
} Frame;

Frame memory[FRAME_COUNT]; // Physischer Speicher als Array
int clock = 0; // Uhr zur Nachverfolgung der Zeit

// Hilfsfunktion, um den physikalischen Speicher zu initialisieren
void initialize_memory() {
    for (int i = 0; i < FRAME_COUNT; i++) {
        memory[i].page_number = -1;
        memory[i].last_used_time = -1;
    }
}

// Funktion, um zu prüfen, ob eine Seite im Speicher ist
bool is_page_in_memory(int page) {
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (memory[i].page_number == page) {
            return true;
        }
    }
    return false;
}

// Funktion, um den Index des zu ersetzenden Frames zu finden (LRU)
int find_frame_to_replace() {
    int lru_index = 0;
    int oldest_time = memory[0].last_used_time;

    for (int i = 1; i < FRAME_COUNT; i++) {
        if (memory[i].last_used_time < oldest_time) {
            oldest_time = memory[i].last_used_time;
            lru_index = i;
        }
    }
    return lru_index;
}

// Funktion, um eine Seite in den Speicher zu laden
void load_page(int page) {
    // Falls die Seite bereits im Speicher ist, aktualisieren wir ihre Zeit
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (memory[i].page_number == page) {
            memory[i].last_used_time = clock;
            return;
        }
    }

    // Falls ein freier Frame vorhanden ist, verwenden wir ihn
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (memory[i].page_number == -1) {
            memory[i].page_number = page;
            memory[i].last_used_time = clock;
            return;
        }
    }

    // Andernfalls finden wir den zu ersetzenden Frame
    int replace_index = find_frame_to_replace();
    memory[replace_index].page_number = page;
    memory[replace_index].last_used_time = clock;
}

// Funktion, um den physischen Speicher anzuzeigen
void display_memory() {
    printf("Frames: ");
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (memory[i].page_number == -1) {
            printf("[ ] ");
        } else {
            printf("[%d] ", memory[i].page_number);
        }
    }
    printf("\n");
}

int main() {
    initialize_memory();

    // Beispielzugriffe auf Seiten
    int page_references[PAGE_COUNT] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3};

    for (int i = 0; i < PAGE_COUNT; i++) {
        printf("Zugriff auf Seite %d:\n", page_references[i]);
        if (!is_page_in_memory(page_references[i])) {
            printf("Seitenfehler! Laden von Seite %d...\n", page_references[i]);
        }
        load_page(page_references[i]);
        display_memory();
        clock++;
    }

    return 0;
}
