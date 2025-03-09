#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define MIN_BLOCK_SIZE 32
#define MAX_BLOCK_SIZE 4096
#define POOL_SIZE (1024 * 1024) // 1 MB

typedef struct BlockHeader {
    size_t size;                // Größe des Blocks
    struct BlockHeader *next;   // Nächster Block (frei)
    struct BlockHeader *prev;   // Vorheriger Block (frei)
    uint32_t magic;             // Magic Number für Double-Free Detection
} BlockHeader;

typedef struct BlockFooter {
    size_t size;                // Größe des Blocks
    uint32_t magic;             // Magic Number für Konsistenzprüfung
} BlockFooter;

static uint8_t memory_pool[POOL_SIZE];
static BlockHeader *free_list = NULL;

#define MAGIC_NUMBER 0xDEADBEEF
#define ALIGN(size) (((size) + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1))
#define HEADER_SIZE ALIGN(sizeof(BlockHeader))
#define FOOTER_SIZE ALIGN(sizeof(BlockFooter))
#define BLOCK_SIZE(size) (HEADER_SIZE + ALIGN(size) + FOOTER_SIZE)

void init_allocator() {
    free_list = (BlockHeader *)memory_pool;
    free_list->size = POOL_SIZE - HEADER_SIZE - FOOTER_SIZE;
    free_list->next = NULL;
    free_list->prev = NULL;
    free_list->magic = MAGIC_NUMBER;

    BlockFooter *footer = (BlockFooter *)((uint8_t *)free_list + HEADER_SIZE + free_list->size);
    footer->size = free_list->size;
    footer->magic = MAGIC_NUMBER;
}

void split_block(BlockHeader *block, size_t size) {
    size_t remaining_size = block->size - size - HEADER_SIZE - FOOTER_SIZE;
    if (remaining_size >= MIN_BLOCK_SIZE) {
        BlockHeader *new_block = (BlockHeader *)((uint8_t *)block + HEADER_SIZE + size + FOOTER_SIZE);
        new_block->size = remaining_size;
        new_block->next = block->next;
        new_block->prev = block;
        new_block->magic = MAGIC_NUMBER;

        BlockFooter *new_footer = (BlockFooter *)((uint8_t *)new_block + HEADER_SIZE + remaining_size);
        new_footer->size = remaining_size;
        new_footer->magic = MAGIC_NUMBER;

        block->size = size;
        block->next = new_block;

        BlockFooter *footer = (BlockFooter *)((uint8_t *)block + HEADER_SIZE + size);
        footer->size = size;
        footer->magic = MAGIC_NUMBER;
    }
}

void *allocate(size_t size) {
    if (size < MIN_BLOCK_SIZE || size > MAX_BLOCK_SIZE) {
        return NULL;
    }

    size = ALIGN(size);
    BlockHeader *current = free_list;

    while (current) {
        if (current->size >= size) {
            if (current == free_list) {
                free_list = current->next;
            }
            if (current->next) {
                current->next->prev = current->prev;
            }
            if (current->prev) {
                current->prev->next = current->next;
            }

            split_block(current, size);
            current->magic = MAGIC_NUMBER;
            return (void *)((uint8_t *)current + HEADER_SIZE);
        }
        current = current->next;
    }

    return NULL;
}

void free_block(void *ptr) {
    if (!ptr) {
        return;
    }

    BlockHeader *block = (BlockHeader *)((uint8_t *)ptr - HEADER_SIZE);

    if (block->magic != MAGIC_NUMBER) {
        fprintf(stderr, "Double free or corruption detected!\n");
        exit(EXIT_FAILURE);
    }

    BlockFooter *footer = (BlockFooter *)((uint8_t *)block + HEADER_SIZE + block->size);
    if (footer->magic != MAGIC_NUMBER) {
        fprintf(stderr, "Corrupted block detected!\n");
        exit(EXIT_FAILURE);
    }

    block->magic = 0; // Mark as free

    BlockHeader *current = free_list;
    while (current && current < block) {
        current = current->next;
    }

    block->next = current;
    if (current) {
        block->prev = current->prev;
        current->prev = block;
    } else {
        block->prev = NULL;
    }

    if (block->prev) {
        block->prev->next = block;
    } else {
        free_list = block;
    }
}

int main() {
    init_allocator();

    void *ptr1 = allocate(128);
    void *ptr2 = allocate(64);
    free_block(ptr1);
    free_block(ptr2);

    return 0;
}
