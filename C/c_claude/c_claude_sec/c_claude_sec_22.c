#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Error codes
typedef enum {
    RING_BUFFER_SUCCESS = 0,
    RING_BUFFER_FULL = -1,
    RING_BUFFER_EMPTY = -2,
    RING_BUFFER_NULL = -3,
    RING_BUFFER_INVALID_SIZE = -4
} ring_buffer_error_t;

typedef struct {
    uint8_t *buffer;                  // Actual data buffer
    size_t capacity;                  // Total capacity of the buffer
    atomic_size_t head;              // Write position
    atomic_size_t tail;              // Read position
    atomic_size_t count;             // Current number of elements
    atomic_flag lock;                // Spinlock for complex operations
} ring_buffer_t;

// Initialize the ring buffer
ring_buffer_error_t ring_buffer_init(ring_buffer_t *rb, size_t capacity) {
    if (!rb) {
        return RING_BUFFER_NULL;
    }
    
    if (capacity == 0 || ((capacity & (capacity - 1)) != 0)) {
        // Capacity must be power of 2
        return RING_BUFFER_INVALID_SIZE;
    }

    rb->buffer = (uint8_t *)calloc(capacity, sizeof(uint8_t));
    if (!rb->buffer) {
        return RING_BUFFER_NULL;
    }

    rb->capacity = capacity;
    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);
    atomic_init(&rb->count, 0);
    atomic_flag_clear(&rb->lock);

    return RING_BUFFER_SUCCESS;
}

// Clean up the ring buffer
void ring_buffer_destroy(ring_buffer_t *rb) {
    if (rb && rb->buffer) {
        free(rb->buffer);
        rb->buffer = NULL;
        rb->capacity = 0;
    }
}

// Write a single byte to the buffer
ring_buffer_error_t ring_buffer_write(ring_buffer_t *rb, uint8_t data) {
    if (!rb || !rb->buffer) {
        return RING_BUFFER_NULL;
    }

    // Check if buffer is full using atomic operation
    size_t current_count = atomic_load(&rb->count);
    if (current_count >= rb->capacity) {
        return RING_BUFFER_FULL;
    }

    // Acquire lock
    while (atomic_flag_test_and_set_explicit(&rb->lock, memory_order_acquire))
        ; // Spin until we get the lock

    // Double-check conditions after acquiring lock
    if (atomic_load(&rb->count) >= rb->capacity) {
        atomic_flag_clear_explicit(&rb->lock, memory_order_release);
        return RING_BUFFER_FULL;
    }

    // Calculate write position
    size_t write_pos = atomic_load(&rb->head) & (rb->capacity - 1);
    
    // Write data
    rb->buffer[write_pos] = data;
    
    // Update head position with memory barrier
    atomic_store_explicit(&rb->head, write_pos + 1, memory_order_release);
    atomic_fetch_add_explicit(&rb->count, 1, memory_order_release);

    // Release lock
    atomic_flag_clear_explicit(&rb->lock, memory_order_release);

    return RING_BUFFER_SUCCESS;
}

// Read a single byte from the buffer
ring_buffer_error_t ring_buffer_read(ring_buffer_t *rb, uint8_t *data) {
    if (!rb || !rb->buffer || !data) {
        return RING_BUFFER_NULL;
    }

    // Check if buffer is empty using atomic operation
    size_t current_count = atomic_load(&rb->count);
    if (current_count == 0) {
        return RING_BUFFER_EMPTY;
    }

    // Acquire lock
    while (atomic_flag_test_and_set_explicit(&rb->lock, memory_order_acquire))
        ; // Spin until we get the lock

    // Double-check conditions after acquiring lock
    if (atomic_load(&rb->count) == 0) {
        atomic_flag_clear_explicit(&rb->lock, memory_order_release);
        return RING_BUFFER_EMPTY;
    }

    // Calculate read position
    size_t read_pos = atomic_load(&rb->tail) & (rb->capacity - 1);
    
    // Read data with memory barrier
    *data = rb->buffer[read_pos];
    
    // Update tail position
    atomic_store_explicit(&rb->tail, read_pos + 1, memory_order_release);
    atomic_fetch_sub_explicit(&rb->count, 1, memory_order_release);

    // Release lock
    atomic_flag_clear_explicit(&rb->lock, memory_order_release);

    return RING_BUFFER_SUCCESS;
}

// Get current number of elements in the buffer
size_t ring_buffer_count(ring_buffer_t *rb) {
    if (!rb) {
        return 0;
    }
    return atomic_load(&rb->count);
}

// Check if buffer is empty
bool ring_buffer_is_empty(ring_buffer_t *rb) {
    if (!rb) {
        return true;
    }
    return atomic_load(&rb->count) == 0;
}

// Check if buffer is full
bool ring_buffer_is_full(ring_buffer_t *rb) {
    if (!rb) {
        return true;
    }
    return atomic_load(&rb->count) >= rb->capacity;
}

#include <stdio.h>

int main() {
    ring_buffer_t rb;
    ring_buffer_error_t err;

    // Initialize buffer with capacity of 16 (power of 2)
    err = ring_buffer_init(&rb, 16);
    if (err != RING_BUFFER_SUCCESS) {
        printf("Failed to initialize ring buffer\n");
        return 1;
    }

    // Write some data
    uint8_t write_data = 42;
    err = ring_buffer_write(&rb, write_data);
    if (err != RING_BUFFER_SUCCESS) {
        printf("Failed to write to ring buffer\n");
        return 1;
    }

    // Read data
    uint8_t read_data;
    err = ring_buffer_read(&rb, &read_data);
    if (err != RING_BUFFER_SUCCESS) {
        printf("Failed to read from ring buffer\n");
        return 1;
    }

    printf("Read data: %d\n", read_data);

    // Clean up
    ring_buffer_destroy(&rb);
    return 0;
}