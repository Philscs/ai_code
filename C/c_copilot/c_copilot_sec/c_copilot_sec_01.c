#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Define the maximum block size
#define MAX_BLOCK_SIZE 4096

// Structure for the boundary tag
typedef struct {
    size_t size;
    bool free;
} boundary_tag_t;

// Structure for the memory pool
typedef struct {
    void* start;
    void* end;
} memory_pool_t;

// Structure for the memory block
typedef struct {
    boundary_tag_t boundary_tag;
    void* data;
} memory_block_t;

// Function to initialize the memory pool
void init_memory_pool(memory_pool_t* pool, void* start, void* end) {
    pool->start = start;
    pool->end = end;

    // Initialize the boundary tag for the entire memory pool
    boundary_tag_t* boundary_tag = (boundary_tag_t*)start;
    boundary_tag->size = (uintptr_t)end - (uintptr_t)start - sizeof(boundary_tag_t);
    boundary_tag->free = true;
}

// Function to allocate a memory block
void* allocate_memory(memory_pool_t* pool, size_t size) {
    // Adjust the size to include the boundary tag
    size_t adjusted_size = size + sizeof(boundary_tag_t);

    // Find the first free block that can accommodate the requested size
    memory_block_t* block = (memory_block_t*)pool->start;
    while ((uintptr_t)block < (uintptr_t)pool->end) {
        if (block->boundary_tag.free && block->boundary_tag.size >= adjusted_size) {
            // Split the block if it's larger than the requested size
            if (block->boundary_tag.size > adjusted_size + sizeof(memory_block_t)) {
                memory_block_t* new_block = (memory_block_t*)((uintptr_t)block + adjusted_size);
                new_block->boundary_tag.size = block->boundary_tag.size - adjusted_size - sizeof(boundary_tag_t);
                new_block->boundary_tag.free = true;
                block->boundary_tag.size = adjusted_size;
            }

            // Mark the block as allocated
            block->boundary_tag.free = false;

            // Return a pointer to the data portion of the block
            return block->data;
        }

        // Move to the next block
        block = (memory_block_t*)((uintptr_t)block + block->boundary_tag.size + sizeof(boundary_tag_t));
    }

    // No suitable block found
    return NULL;
}

// Function to free a memory block
void free_memory(memory_pool_t* pool, void* ptr) {
    // Find the block corresponding to the given pointer
    memory_block_t* block = (memory_block_t*)((uintptr_t)ptr - sizeof(boundary_tag_t));

    // Mark the block as free
    block->boundary_tag.free = true;

    // Perform double-free detection
    memory_block_t* next_block = (memory_block_t*)((uintptr_t)block + block->boundary_tag.size + sizeof(boundary_tag_t));
    if ((uintptr_t)next_block < (uintptr_t)pool->end && next_block->boundary_tag.free) {
        // Merge the current block with the next block
        block->boundary_tag.size += next_block->boundary_tag.size + sizeof(boundary_tag_t);
    }
}

// Example usage
int main() {
    // Define the memory pool
    uint8_t memory_pool[MAX_BLOCK_SIZE];

    // Initialize the memory pool
    memory_pool_t pool;
    init_memory_pool(&pool, memory_pool, memory_pool + sizeof(memory_pool));

    // Allocate a block of memory
    void* ptr = allocate_memory(&pool, 64);

    // Use the allocated memory
    if (ptr != NULL) {
        // ...
    }

    // Free the allocated memory
    free_memory(&pool, ptr);

    return 0;
}
