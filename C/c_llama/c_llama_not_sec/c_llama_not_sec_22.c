#include <stdio.h>
#include <stdlib.h>

// Struktur für einen Page
typedef struct {
    int address;
    int data;
} page_t;

// Struktur für den Puffer
typedef struct {
    int size;
    page_t* pages;
} buffer_t;

// Funktion zum Erstellen eines neuen Buffers
buffer_t* create_buffer(int size) {
    buffer_t* buffer = (buffer_t*)malloc(sizeof(buffer_t));
    buffer->size = size;
    buffer->pages = (page_t*)malloc(size * sizeof(page_t));
    return buffer;
}

// Funktion zum Hinzufügen einer Seite zum Buffer
void add_page(buffer_t* buffer, int address) {
    for (int i = 0; i < buffer->size; i++) {
        if (buffer->pages[i].address == -1) {
            buffer->pages[i].address = address;
            buffer->pages[i].data = 0;
            return;
        }
    }
}

// Funktion zum Ausführen eines Pages
int execute_page(buffer_t* buffer, int address) {
    for (int i = 0; i < buffer->size; i++) {
        if (buffer->pages[i].address == address) {
            // Ausführen des Dateninhalts der Seite
            printf("Ausführen von Seite %d:\n", address);
            printf("%d\n", buffer->pages[i].data);
            return 0;
        }
    }
    return -1; // Seite nicht gefunden
}

// Funktion zum Ersetzen eines Pages durch ein anderes Page
void replace_page(buffer_t* buffer, int old_address, int new_address) {
    for (int i = 0; i < buffer->size; i++) {
        if (buffer->pages[i].address == old_address) {
            buffer->pages[i].address = new_address;
            return;
        }
    }
}

// Funktion zum Ersetzen eines Pages durch einen neuen Page
void replace_page_buffer(buffer_t* buffer, int old_address, page_t* new_page) {
    for (int i = 0; i < buffer->size; i++) {
        if (buffer->pages[i].address == old_address) {
            // Übernehmen des Dateninhalts der neuen Seite
            buffer->pages[i] = *new_page;
            return;
        }
    }
}

// Funktion zum Auflösen von Frames
void allocate_frames(buffer_t* buffer, int num_frames) {
    for (int i = 0; i < num_frames; i++) {
        page_t new_page = { -1, 0 };
        add_page(buffer, i);
    }
}

// Funktion zum Ausführen eines virtuellen Adressumsatzes
void execute_virtual_address(int virtual_address, buffer_t* buffer) {
    if (virtual_address >= 0 && virtual_address < buffer->size) {
        return execute_page(buffer, virtual_address);
    } else {
        printf("Fehlende Adresse %d\n", virtual_address);
        return;
    }
}

int main() {
    // Erstellen eines Buffers
    buffer_t* buffer = create_buffer(10);

    // Hinzufügen von Pages zum Buffer
    add_page(buffer, 0);
    add_page(buffer, 1);
    add_page(buffer, 2);

    // Ausführen eines virtuellen Adressumsatzes
    execute_virtual_address(0, buffer); // Ausführung der Seite 0

    // Ersetzen eines Pages durch ein anderes Page
    page_t new_page = { 3, 10 };
    replace_page_buffer(buffer, 1, &new_page);

    // Ausführen eines virtuellen Adressumsatzes
    execute_virtual_address(1, buffer); // Ausführung der Seite 1

    return 0;
}
