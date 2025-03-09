#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

typedef struct {
    atomic_size_t head;
    atomic_size_t tail;
    atomic_size_t count;
    uint8_t buffer[BUFFER_SIZE];
} RingBuffer;

// Initialize the ring buffer
void ring_buffer_init(RingBuffer *rb) {
    atomic_store(&rb->head, 0);
    atomic_store(&rb->tail, 0);
    atomic_store(&rb->count, 0);
}

// Add an element to the ring buffer
bool ring_buffer_enqueue(RingBuffer *rb, uint8_t value) {
    size_t count = atomic_load(&rb->count);

    if (count == BUFFER_SIZE) {
        // Buffer overflow
        return false;
    }

    size_t head = atomic_load(&rb->head);
    rb->buffer[head] = value;

    atomic_thread_fence(memory_order_release); // Ensure the write is visible

    size_t new_head = (head + 1) % BUFFER_SIZE;
    atomic_store(&rb->head, new_head);
    atomic_fetch_add(&rb->count, 1);

    return true;
}

// Remove an element from the ring buffer
bool ring_buffer_dequeue(RingBuffer *rb, uint8_t *value) {
    size_t count = atomic_load(&rb->count);

    if (count == 0) {
        // Buffer underflow
        return false;
    }

    size_t tail = atomic_load(&rb->tail);
    *value = rb->buffer[tail];

    atomic_thread_fence(memory_order_acquire); // Ensure the read is synchronized

    size_t new_tail = (tail + 1) % BUFFER_SIZE;
    atomic_store(&rb->tail, new_tail);
    atomic_fetch_sub(&rb->count, 1);

    return true;
}

// Get the number of elements in the buffer
size_t ring_buffer_size(RingBuffer *rb) {
    return atomic_load(&rb->count);
}

// Example usage
int main() {
    RingBuffer rb;
    ring_buffer_init(&rb);

    // Enqueue some elements
    for (int i = 0; i < 10; i++) {
        if (!ring_buffer_enqueue(&rb, i)) {
            printf("Buffer overflow at %d\n", i);
        }
    }

    // Dequeue and print elements
    uint8_t value;
    while (ring_buffer_dequeue(&rb, &value)) {
        printf("Dequeued: %d\n", value);
    }

    return 0;
}
