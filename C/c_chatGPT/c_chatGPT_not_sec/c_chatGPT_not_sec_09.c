#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// Ring Buffer Struktur
typedef struct {
    atomic_size_t head;
    atomic_size_t tail;
    size_t capacity;
    void **buffer;
} RingBuffer;

// Initialisierung des Ring Buffers
RingBuffer *ring_buffer_init(size_t capacity) {
    RingBuffer *rb = malloc(sizeof(RingBuffer));
    if (!rb) return NULL;

    rb->buffer = malloc(capacity * sizeof(void *));
    if (!rb->buffer) {
        free(rb);
        return NULL;
    }

    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);
    rb->capacity = capacity;

    return rb;
}

// Freigeben des Ring Buffers
void ring_buffer_free(RingBuffer *rb) {
    if (rb) {
        free(rb->buffer);
        free(rb);
    }
}

// Hinzufügen eines Elements (Multi-Producer sicher)
bool ring_buffer_enqueue(RingBuffer *rb, void *item) {
    size_t head, tail;

    do {
        head = atomic_load(&rb->head);
        tail = atomic_load(&rb->tail);

        if ((head + 1) % rb->capacity == tail) {
            // Buffer ist voll
            return false;
        }
    } while (!atomic_compare_exchange_weak(&rb->head, &head, (head + 1) % rb->capacity));

    rb->buffer[head] = item;
    return true;
}

// Entfernen eines Elements (Multi-Consumer sicher)
bool ring_buffer_dequeue(RingBuffer *rb, void **item) {
    size_t head, tail;

    do {
        tail = atomic_load(&rb->tail);
        head = atomic_load(&rb->head);

        if (tail == head) {
            // Buffer ist leer
            return false;
        }
    } while (!atomic_compare_exchange_weak(&rb->tail, &tail, (tail + 1) % rb->capacity));

    *item = rb->buffer[tail];
    return true;
}

// Beispielnutzung
int main() {
    size_t capacity = 8;
    RingBuffer *rb = ring_buffer_init(capacity);

    if (!rb) {
        fprintf(stderr, "Fehler beim Initialisieren des Ring Buffers\n");
        return EXIT_FAILURE;
    }

    // Beispiel: Produzieren und Konsumieren
    int data1 = 42, data2 = 43;
    void *item;

    if (ring_buffer_enqueue(rb, &data1)) {
        printf("Enqueued: %d\n", data1);
    } else {
        printf("Enqueue fehlgeschlagen\n");
    }

    if (ring_buffer_enqueue(rb, &data2)) {
        printf("Enqueued: %d\n", data2);
    } else {
        printf("Enqueue fehlgeschlagen\n");
    }

    if (ring_buffer_dequeue(rb, &item)) {
        printf("Dequeued: %d\n", *(int *)item);
    } else {
        printf("Dequeue fehlgeschlagen\n");
    }

    if (ring_buffer_dequeue(rb, &item)) {
        printf("Dequeued: %d\n", *(int *)item);
    } else {
        printf("Dequeue fehlgeschlagen\n");
    }

    ring_buffer_free(rb);
    return EXIT_SUCCESS;
}
