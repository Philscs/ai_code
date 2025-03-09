#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#define BUFFER_SIZE 10

typedef struct {
    atomic_size_t read_index;
    atomic_size_t write_index;
    void* buffer[BUFFER_SIZE];
} RingBuffer;

void ring_buffer_init(RingBuffer* buffer) {
    atomic_init(&buffer->read_index, 0);
    atomic_init(&buffer->write_index, 0);
}

bool ring_buffer_push(RingBuffer* buffer, void* data) {
    size_t write_index = atomic_load_explicit(&buffer->write_index, memory_order_relaxed);
    size_t next_write_index = (write_index + 1) % BUFFER_SIZE;

    if (next_write_index == atomic_load_explicit(&buffer->read_index, memory_order_acquire)) {
        return false; // Buffer is full
    }

    buffer->buffer[write_index] = data;
    atomic_store_explicit(&buffer->write_index, next_write_index, memory_order_release);

    return true;
}

bool ring_buffer_pop(RingBuffer* buffer, void** data) {
    size_t read_index = atomic_load_explicit(&buffer->read_index, memory_order_relaxed);

    if (read_index == atomic_load_explicit(&buffer->write_index, memory_order_acquire)) {
        return false; // Buffer is empty
    }

    *data = buffer->buffer[read_index];
    size_t next_read_index = (read_index + 1) % BUFFER_SIZE;
    atomic_store_explicit(&buffer->read_index, next_read_index, memory_order_release);

    return true;
}
