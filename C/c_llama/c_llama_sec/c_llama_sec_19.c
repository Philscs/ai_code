#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Strukt für einen Block mit einer bestimmten Größe
typedef struct {
    void* addr;
    size_t size;
    bool is_used;
    bool is_free;
} block_t;

// Struktur für den Heap
typedef struct {
    block_t* free_list;
    pthread_mutex_t lock;
} heap_t;

// Funktion, um einen neuen Block zu erstellen
void* create_block(heap_t* h, size_t size) {
    // Erstelle einen neuen Block
    block_t* new_block = malloc(sizeof(block_t));
    if (!new_block) {
        return NULL;
    }
    new_block->addr = malloc(size);
    new_block->size = size;
    new_block->is_used = false;
    new_block->is_free = true;

    // Füge den Block zum Heap hinzu
    pthread_mutex_lock(&h->lock);
    if (h->free_list) {
        block_t* first_free_block = h->free_list;
        while (first_free_block && first_free_block->size < size) {
            first_free_block = first_free_block->next;
        }
        if (first_free_block == h->free_list || first_free_block->size > size + sizeof(block_t)) 
{
            new_block->addr = first_free_block->addr;
            free(first_free_block);
            h->free_list = new_block->next;
            break;
        } else {
            // Wenn der erste frei verfügbare Block nicht groß genug ist,
            // dann füge den neuen Block zur Liste hinzu
            pthread_mutex_unlock(&h->lock);
            block_t* new_next = malloc(sizeof(block_t));
            new_next->addr = first_free_block->addr + sizeof(block_t) + size;
            new_next->size = first_free_block->size - (sizeof(block_t) + size);
            new_next->is_used = false;
            new_next->is_free = true;
            pthread_mutex_lock(&h->lock);
            h->free_list = new_next;
            break;
        }
    } else {
        // Wenn der erste frei verfügbare Block nicht existiert,
        // dann füge den neuen Block zur Liste hinzu
        block_t* new_next = malloc(sizeof(block_t));
        new_next->addr = NULL;
        new_next->size = 0;
        new_next->is_used = false;
        new_next->is_free = true;
        pthread_mutex_lock(&h->lock);
        h->free_list = new_next;
    }
    pthread_mutex_unlock(&h->lock);

    return (void*)new_block;
}

// Funktion, um einen Block zu freien
void free_block(heap_t* h, void* addr) {
    block_t* current_block = (block_t*)((char*)addr - sizeof(block_t));
    if (!current_block || !current_block->is_free) {
        return;
    }

    pthread_mutex_lock(&h->lock);
    // Füge den Block zum Heap hinzu
    if (current_block->next == h->free_list) {
        h->free_list = current_block->addr + sizeof(block_t) + current_block->size;
    } else {
        block_t* prev = NULL;
        block_t* current = h->free_list;
        while (current && current != current_block->addr + sizeof(block_t)) {
            if (current == current_block->next) {
                current->next = h->free_list;
                break;
            }
            prev = current;
            current = current->next;
        }
        if (prev) {
            prev->next = current_block->next;
        } else {
            h->free_list = current_block->next;
        }
    }

    free(current_block->addr);
    free(current_block);

    pthread_mutex_unlock(&h->lock);
}

// Funktion, um einen spezifischen Block zu finden
void* find_block(heap_t* h, void* addr) {
    block_t* current_block = (block_t*)((char*)addr - sizeof(block_t));
    if (!current_block || !current_block->is_free) {
        return NULL;
    }

    pthread_mutex_lock(&h->lock);
    // Füge den Block zum Heap hinzu
    while (current_block && current_block != h->free_list) {
        if (current_block->addr == addr) {
            break;
        }
        block_t* next = current_block->next;
        current_block = next;
    }

    pthread_mutex_unlock(&h->lock);

    return addr;
}

// Funktion, um einen spezifischen Block zu finden
void* find_block_addr(heap_t* h, size_t size) {
    block_t* current_block = h->free_list;
    while (current_block && current_block->size != size) {
        current_block = current_block->next;
    }

    if (!current_block || !current_block->is_free) {
        return NULL;
    }

    pthread_mutex_lock(&h->lock);
    // Füge den Block zum Heap hinzu
    void* addr = malloc(size + sizeof(block_t));
    memcpy(addr, (char*)current_block->addr - sizeof(block_t), size);
    free_block(h, current_block->addr);
    free(current_block);

    pthread_mutex_unlock(&h->lock);

    return addr;
}

// Funktion, um einen neuen Block zu erstellen
void* custom_allocator(size_t size) {
    heap_t heap;
    heap.free_list = NULL;
    pthread_mutex_init(&heap.lock, NULL);

    void* new_block = create_block(&heap, size);
    if (new_block == NULL) {
        return NULL;
    }

    void* addr = malloc(size);
    memcpy(addr, new_block->addr + sizeof(block_t), size - sizeof(block_t));

    free_block(&heap, new_block->addr);
    free(new_block);

    pthread_mutex_destroy(&heap.lock);
    return addr;
}

int main() {
    // Erstelle einen neuen Block mit einer Größe von 10 Bytes
    void* block = custom_allocator(10);

    if (block) {
        printf("Block erstellt erfolgreich\n");
    } else {
        printf("Fehler beim Erstellen eines Blocks\n");
    }

    return 0;
}