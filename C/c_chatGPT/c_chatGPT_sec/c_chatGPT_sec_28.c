#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <pthread.h>

#define MAGIC_FREE 0xDEADBEEF
#define BUFFER_PADDING 16

typedef struct MemoryBlock {
    void *address;
    size_t size;
    char *file;
    int line;
    void *call_stack[10];
    int stack_depth;
    int freed;
    struct MemoryBlock *next;
} MemoryBlock;

static MemoryBlock *head = NULL;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void add_memory_block(void *ptr, size_t size, const char *file, int line) {
    pthread_mutex_lock(&lock);

    MemoryBlock *block = (MemoryBlock *)malloc(sizeof(MemoryBlock));
    block->address = ptr;
    block->size = size;
    block->file = strdup(file);
    block->line = line;
    block->freed = 0;
    block->stack_depth = backtrace(block->call_stack, 10);
    block->next = head;
    head = block;

    pthread_mutex_unlock(&lock);
}

MemoryBlock *find_memory_block(void *ptr) {
    MemoryBlock *current = head;
    while (current) {
        if (current->address == ptr) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void remove_memory_block(void *ptr) {
    pthread_mutex_lock(&lock);

    MemoryBlock *current = head;
    MemoryBlock *prev = NULL;
    while (current) {
        if (current->address == ptr) {
            if (prev) {
                prev->next = current->next;
            } else {
                head = current->next;
            }
            free(current->file);
            free(current);
            pthread_mutex_unlock(&lock);
            return;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&lock);
}

void *custom_malloc(size_t size, const char *file, int line) {
    size_t total_size = size + 2 * BUFFER_PADDING;
    unsigned char *raw = (unsigned char *)malloc(total_size);
    if (!raw) {
        return NULL;
    }

    memset(raw, 0xAA, BUFFER_PADDING); // Padding at the start
    memset(raw + BUFFER_PADDING + size, 0xBB, BUFFER_PADDING); // Padding at the end
    void *user_ptr = raw + BUFFER_PADDING;

    add_memory_block(user_ptr, size, file, line);
    return user_ptr;
}

void custom_free(void *ptr, const char *file, int line) {
    if (!ptr) return;

    pthread_mutex_lock(&lock);
    MemoryBlock *block = find_memory_block(ptr);
    if (!block) {
        fprintf(stderr, "[ERROR] Attempt to free unallocated memory at %s:%d\n", file, line);
        pthread_mutex_unlock(&lock);
        return;
    }

    if (block->freed) {
        fprintf(stderr, "[ERROR] Use-after-free detected at %s:%d\n", file, line);
        pthread_mutex_unlock(&lock);
        return;
    }

    unsigned char *raw = (unsigned char *)block->address - BUFFER_PADDING;
    for (size_t i = 0; i < BUFFER_PADDING; ++i) {
        if (raw[i] != 0xAA || raw[BUFFER_PADDING + block->size + i] != 0xBB) {
            fprintf(stderr, "[ERROR] Buffer overflow detected for block allocated at %s:%d\n", block->file, block->line);
            break;
        }
    }

    block->freed = 1;
    memset(ptr, MAGIC_FREE, block->size);
    free(raw);

    pthread_mutex_unlock(&lock);
}

void generate_report() {
    pthread_mutex_lock(&lock);

    MemoryBlock *current = head;
    int leaks = 0;

    printf("Memory Debug Report:\n");
    while (current) {
        if (!current->freed) {
            ++leaks;
            printf("Leaked memory at %p (size: %zu, allocated at %s:%d)\n",
                   current->address, current->size, current->file, current->line);
            printf("Stack trace:\n");
            char **symbols = backtrace_symbols(current->call_stack, current->stack_depth);
            for (int i = 0; i < current->stack_depth; ++i) {
                printf("  %s\n", symbols[i]);
            }
            free(symbols);
        }
        current = current->next;
    }

    if (leaks == 0) {
        printf("No memory leaks detected.\n");
    }

    pthread_mutex_unlock(&lock);
}

#define malloc(size) custom_malloc(size, __FILE__, __LINE__)
#define free(ptr) custom_free(ptr, __FILE__, __LINE__)

int main() {
    char *data1 = (char *)malloc(100);
    strcpy(data1, "Test memory allocation.");
    printf("Data1: %s\n", data1);
    free(data1);

    // Simulate leak
    char *data2 = (char *)malloc(50);

    // Simulate buffer overflow
    char *data3 = (char *)malloc(10);
    strcpy(data3, "Overflow!");

    generate_report();

    return 0;
}
