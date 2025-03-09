#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_ALLOCATIONS 10000
#define MAX_STACK_DEPTH 50
#define CANARY_VALUE 0xDEADBEEF
#define BUFFER_PADDING 8

// Structure to store allocation information
typedef struct {
    void* ptr;
    size_t size;
    char* file;
    int line;
    void* stack_trace[MAX_STACK_DEPTH];
    int stack_depth;
    bool freed;
    time_t timestamp;
} AllocationInfo;

// Global state
static AllocationInfo allocations[MAX_ALLOCATIONS];
static int allocation_count = 0;
static FILE* log_file = NULL;

// Initialize the debugger
void debug_init(void) {
    log_file = fopen("memory_debug.log", "w");
    if (!log_file) {
        fprintf(stderr, "Failed to open debug log file\n");
        exit(1);
    }
    
    fprintf(log_file, "Memory Debugger Initialized at %s\n", ctime(&(time_t){time(NULL)}));
}

// Get stack trace
static void capture_stack_trace(AllocationInfo* info) {
    info->stack_depth = backtrace(info->stack_trace, MAX_STACK_DEPTH);
}

// Custom malloc wrapper
void* debug_malloc(size_t size, const char* file, int line) {
    if (allocation_count >= MAX_ALLOCATIONS) {
        fprintf(log_file, "ERROR: Maximum allocations reached\n");
        return NULL;
    }

    // Allocate extra space for buffer overflow detection
    size_t total_size = size + (2 * BUFFER_PADDING);
    void* ptr = malloc(total_size);
    if (!ptr) return NULL;

    // Set canary values
    unsigned int* canary_start = (unsigned int*)ptr;
    unsigned int* canary_end = (unsigned int*)(ptr + BUFFER_PADDING + size);
    *canary_start = CANARY_VALUE;
    *canary_end = CANARY_VALUE;

    // Record allocation
    AllocationInfo* info = &allocations[allocation_count++];
    info->ptr = ptr + BUFFER_PADDING;  // Return pointer after the canary
    info->size = size;
    info->file = strdup(file);
    info->line = line;
    info->freed = false;
    info->timestamp = time(NULL);
    capture_stack_trace(info);

    fprintf(log_file, "Allocation: %p, Size: %zu, File: %s, Line: %d\n", 
            info->ptr, size, file, line);

    return info->ptr;
}

// Custom free wrapper
void debug_free(void* ptr, const char* file, int line) {
    if (!ptr) return;

    // Find allocation info
    AllocationInfo* info = NULL;
    for (int i = 0; i < allocation_count; i++) {
        if (allocations[i].ptr == ptr) {
            info = &allocations[i];
            break;
        }
    }

    if (!info) {
        fprintf(log_file, "ERROR: Attempting to free unallocated pointer %p at %s:%d\n",
                ptr, file, line);
        return;
    }

    if (info->freed) {
        fprintf(log_file, "ERROR: Double free detected at %s:%d\n", file, line);
        fprintf(log_file, "Original allocation at %s:%d\n", info->file, info->line);
        return;
    }

    // Check for buffer overflow
    unsigned int* canary_start = (unsigned int*)(ptr - BUFFER_PADDING);
    unsigned int* canary_end = (unsigned int*)(ptr + info->size);
    
    if (*canary_start != CANARY_VALUE || *canary_end != CANARY_VALUE) {
        fprintf(log_file, "ERROR: Buffer overflow detected for allocation at %s:%d\n",
                info->file, info->line);
        print_stack_trace(info);
    }

    info->freed = true;
    free(canary_start);  // Free the actual start of the allocation
}

// Check for memory leaks
void check_leaks(void) {
    fprintf(log_file, "\n=== Memory Leak Report ===\n");
    int leak_count = 0;
    
    for (int i = 0; i < allocation_count; i++) {
        if (!allocations[i].freed) {
            leak_count++;
            fprintf(log_file, "Leak #%d:\n", leak_count);
            fprintf(log_file, "  Address: %p\n", allocations[i].ptr);
            fprintf(log_file, "  Size: %zu bytes\n", allocations[i].size);
            fprintf(log_file, "  Allocated at: %s:%d\n", 
                    allocations[i].file, allocations[i].line);
            fprintf(log_file, "  Time: %s", ctime(&allocations[i].timestamp));
            print_stack_trace(&allocations[i]);
        }
    }
    
    fprintf(log_file, "\nTotal leaks found: %d\n", leak_count);
}

// Print stack trace
void print_stack_trace(AllocationInfo* info) {
    fprintf(log_file, "Stack trace:\n");
    char** symbols = backtrace_symbols(info->stack_trace, info->stack_depth);
    
    if (symbols) {
        for (int i = 0; i < info->stack_depth; i++) {
            fprintf(log_file, "  %s\n", symbols[i]);
        }
        free(symbols);
    }
}

// Use-after-free detection
bool is_memory_valid(void* ptr) {
    for (int i = 0; i < allocation_count; i++) {
        if (allocations[i].ptr == ptr) {
            if (allocations[i].freed) {
                fprintf(log_file, "ERROR: Use-after-free detected for pointer %p\n", ptr);
                fprintf(log_file, "Originally allocated at %s:%d\n", 
                        allocations[i].file, allocations[i].line);
                print_stack_trace(&allocations[i]);
                return false;
            }
            return true;
        }
    }
    return false;
}

// Generate final report
void generate_report(void) {
    fprintf(log_file, "\n=== Memory Debugger Final Report ===\n");
    fprintf(log_file, "Total allocations: %d\n", allocation_count);
    
    int active_allocations = 0;
    size_t total_memory = 0;
    
    for (int i = 0; i < allocation_count; i++) {
        if (!allocations[i].freed) {
            active_allocations++;
            total_memory += allocations[i].size;
        }
    }
    
    fprintf(log_file, "Active allocations: %d\n", active_allocations);
    fprintf(log_file, "Total memory in use: %zu bytes\n", total_memory);
    
    check_leaks();
    
    if (log_file) {
        fclose(log_file);
    }
}

// Cleanup function
void debug_cleanup(void) {
    generate_report();
    
    // Free allocation info
    for (int i = 0; i < allocation_count; i++) {
        free(allocations[i].file);
    }
}

// Macro definitions for easier usage
#define malloc(size) debug_malloc(size, __FILE__, __LINE__)
#define free(ptr) debug_free(ptr, __FILE__, __LINE__)

// Example usage
int main() {
    debug_init();
    
    // Example allocations
    char* str1 = malloc(100);
    char* str2 = malloc(50);
    
    // Buffer overflow example
    strcpy(str1, "This is a very long string that might cause buffer overflow");
    
    // Memory leak (not freeing str2)
    free(str1);
    
    // Use-after-free example
    str1[0] = 'A';  // This should trigger a use-after-free detection
    
    debug_cleanup();
    return 0;
}