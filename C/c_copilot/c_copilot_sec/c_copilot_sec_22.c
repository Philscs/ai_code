#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>

#define BUFFER_SIZE 10

typedef struct {
    int buffer[BUFFER_SIZE];
    atomic_int read_index;
    atomic_int write_index;
} RingBuffer;

bool is_buffer_full(RingBuffer* buffer) {
    return (atomic_load_explicit(&buffer->write_index, memory_order_acquire) + 1) % BUFFER_SIZE == atomic_load_explicit(&buffer->read_index, memory_order_acquire);
}

bool is_buffer_empty(RingBuffer* buffer) {
    return atomic_load_explicit(&buffer->read_index, memory_order_acquire) == atomic_load_explicit(&buffer->write_index, memory_order_acquire);
}

bool enqueue(RingBuffer* buffer, int value) {
    if (is_buffer_full(buffer)) {
        fprintf(stderr, "Error: Buffer is full\n");
        return false;
    }

    int write_index = atomic_load_explicit(&buffer->write_index, memory_order_relaxed);
    buffer->buffer[write_index] = value;
    atomic_store_explicit(&buffer->write_index, (write_index + 1) % BUFFER_SIZE, memory_order_release);

    return true;
}

bool dequeue(RingBuffer* buffer, int* value) {
    if (is_buffer_empty(buffer)) {
        fprintf(stderr, "Error: Buffer is empty\n");
        return false;
    }

    int read_index = atomic_load_explicit(&buffer->read_index, memory_order_relaxed);
    *value = buffer->buffer[read_index];
    atomic_store_explicit(&buffer->read_index, (read_index + 1) % BUFFER_SIZE, memory_order_release);

    return true;
}

int main() {
    RingBuffer buffer;
    atomic_init(&buffer.read_index, 0);
    atomic_init(&buffer.write_index, 0);

    // Example usage
    enqueue(&buffer, 10);
    enqueue(&buffer, 20);

    int value;
    dequeue(&buffer, &value);
    printf("Dequeued value: %d\n", value);

    return 0;
}
