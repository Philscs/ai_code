#include <stdatomic.h>
#include <stdbool.h>

// Struktur für den Ring Buffer
typedef struct {
    atomic_size_t head;
    atomic_size_t tail;
} ring_buffer_t;

// Funktion zur Erstellung des Ring Buffers
ring_buffer_t* create_ring_buffer(size_t size) {
    ring_buffer_t* buffer = malloc(sizeof(ring_buffer_t));
    if (buffer == NULL) {
        return NULL; // Kein Speicher für den Buffer verfügbar
    }

    atomic_init(&buffer->head, 0);
    atomic_init(&buffer->tail, 0);

    return buffer;
}

// Funktion zum Hinzufügen eines Elements in den Ring Buffer
bool ring_buffer_push(ring_buffer_t* buffer, void* data) {
    size_t current_tail = atomic_fetch_add(&buffer->tail, 1);

    while (atomic_compare_exchange_strong(&buffer->head, &current_tail,
                                            current_tail + 1)) {
        // Wenn der Buffer voll ist, warten
        if (current_tail == buffer->tail) {
            return false; // Buffer ist voll
        }
    }

    return true; // Daten erfolgreich in den Buffer geschrieben
}

// Funktion zum Abholen eines Elements aus dem Ring Buffer
bool ring_buffer_pop(ring_buffer_t* buffer, void** data) {
    size_t current_head = atomic_fetch_add(&buffer->head, 1);

    while (atomic_compare_exchange_strong(&current_head,
                                            &buffer->tail,
                                            current_head - 1)) {
        // Wenn der Buffer leer ist, warten
        if (current_head == buffer->head) {
            return false; // Buffer ist leer
        }
    }

    atomic_store(data, &buffer->tail);
    return true; // Daten erfolgreich aus dem Buffer gelesen
}

// Funktion zum Zerstören des Ring Buffers
void ring_buffer_destroy(ring_buffer_t* buffer) {
    free(buffer);
}

int main() {
    // Erstellung eines Ring Buffers mit einer Kapazität von 10 Elementen
    ring_buffer_t* buffer = create_ring_buffer(10);

    if (buffer == NULL) {
        return EXIT_FAILURE; // Kein Speicher für den Buffer verfügbar
    }

    // Daten hinzufügen
    int data[5];
    for (int i = 0; i < 5; i++) {
        ring_buffer_push(buffer, &data[i]);
        printf("Data %d eingefügt\n", i);
    }

    // Daten abholen und ausgeben
    for (int i = 0; i < 5; i++) {
        int* data_ptr;
        if (!ring_buffer_pop(buffer, &data_ptr)) {
            return EXIT_FAILURE; // Buffer ist leer oder voll
        }
        printf("Data %d ausgelesen: %d\n", i, *data_ptr);
    }

    // Zerstören des Ring Buffers
    ring_buffer_destroy(buffer);

    return EXIT_SUCCESS;
}
