#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void** buffer;           // Der eigentliche Ringpuffer
    size_t capacity;         // Kapazität des Puffers
    atomic_size_t head;      // Index für das Schreiben
    atomic_size_t tail;      // Index für das Lesen
    atomic_size_t* slots;    // Sequenznummern für jeden Slot
} RingBuffer;

RingBuffer* ring_buffer_create(size_t capacity) {
    RingBuffer* rb = (RingBuffer*)malloc(sizeof(RingBuffer));
    if (!rb) return NULL;

    // Allokiere Speicher für Buffer und Slots
    rb->buffer = (void**)malloc(capacity * sizeof(void*));
    rb->slots = (atomic_size_t*)malloc(capacity * sizeof(atomic_size_t));
    
    if (!rb->buffer || !rb->slots) {
        free(rb->buffer);
        free(rb->slots);
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);

    // Initialisiere Sequenznummern
    for (size_t i = 0; i < capacity; i++) {
        atomic_init(&rb->slots[i], i);
    }

    return rb;
}

void ring_buffer_destroy(RingBuffer* rb) {
    if (rb) {
        free(rb->buffer);
        free(rb->slots);
        free(rb);
    }
}

bool ring_buffer_push(RingBuffer* rb, void* item) {
    size_t head, next;
    size_t seq;

    do {
        head = atomic_load_explicit(&rb->head, memory_order_relaxed);
        seq = atomic_load_explicit(&rb->slots[head % rb->capacity], 
                                 memory_order_acquire);

        // Überprüfe ob der Slot verfügbar ist
        if (seq != head) {
            return false;  // Buffer ist voll
        }

        next = head + 1;
        // Versuche head zu aktualisieren
    } while (!atomic_compare_exchange_weak_explicit(
        &rb->head, &head, next,
        memory_order_relaxed,
        memory_order_relaxed));

    // Schreibe das Item in den Buffer
    rb->buffer[head % rb->capacity] = item;
    
    // Markiere den Slot als verfügbar für Consumer
    atomic_store_explicit(&rb->slots[head % rb->capacity], 
                         head + rb->capacity,
                         memory_order_release);

    return true;
}

bool ring_buffer_pop(RingBuffer* rb, void** item) {
    size_t tail, next;
    size_t seq;

    do {
        tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
        seq = atomic_load_explicit(&rb->slots[tail % rb->capacity],
                                 memory_order_acquire);

        // Überprüfe ob Daten verfügbar sind
        if (seq != tail + rb->capacity) {
            return false;  // Buffer ist leer
        }

        next = tail + 1;
        // Versuche tail zu aktualisieren
    } while (!atomic_compare_exchange_weak_explicit(
        &rb->tail, &tail, next,
        memory_order_relaxed,
        memory_order_relaxed));

    // Lese das Item aus dem Buffer
    *item = rb->buffer[tail % rb->capacity];
    
    // Markiere den Slot als verfügbar für Producer
    atomic_store_explicit(&rb->slots[tail % rb->capacity],
                         tail + rb->capacity,
                         memory_order_release);

    return true;
}