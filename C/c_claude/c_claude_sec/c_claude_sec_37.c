#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_BLOCKS 1024
#define MAX_LOG_MESSAGE 256
#define LOG_FILE "memory_compactor.log"

// Memory block structure
typedef struct {
    void* start;
    size_t size;
    bool is_free;
    pthread_mutex_t block_mutex;
} MemoryBlock;

// Memory compactor structure
typedef struct {
    void* memory_pool;
    size_t total_size;
    MemoryBlock blocks[MAX_BLOCKS];
    int block_count;
    pthread_mutex_t compactor_mutex;
    FILE* log_file;
} MemoryCompactor;

// Error codes
typedef enum {
    MC_SUCCESS = 0,
    MC_ERROR_INITIALIZATION,
    MC_ERROR_ALLOCATION,
    MC_ERROR_INVALID_PARAMETER,
    MC_ERROR_COMPACTION,
    MC_ERROR_THREAD_SAFETY
} MemoryCompactorError;

// Logging function
static void log_message(MemoryCompactor* compactor, const char* message, const char* level) {
    if (!compactor->log_file) return;

    time_t now;
    time(&now);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';  // Remove newline

    pthread_mutex_lock(&compactor->compactor_mutex);
    fprintf(compactor->log_file, "[%s] [%s] %s\n", timestamp, level, message);
    fflush(compactor->log_file);
    pthread_mutex_unlock(&compactor->compactor_mutex);
}

// Initialize memory compactor
MemoryCompactorError init_compactor(MemoryCompactor* compactor, size_t size) {
    compactor->memory_pool = malloc(size);
    if (!compactor->memory_pool) {
        return MC_ERROR_INITIALIZATION;
    }

    compactor->total_size = size;
    compactor->block_count = 1;
    
    // Initialize first block as free
    compactor->blocks[0].start = compactor->memory_pool;
    compactor->blocks[0].size = size;
    compactor->blocks[0].is_free = true;
    pthread_mutex_init(&compactor->blocks[0].block_mutex, NULL);

    // Initialize compactor mutex
    pthread_mutex_init(&compactor->compactor_mutex, NULL);

    // Initialize logging
    compactor->log_file = fopen(LOG_FILE, "a");
    if (!compactor->log_file) {
        free(compactor->memory_pool);
        return MC_ERROR_INITIALIZATION;
    }

    log_message(compactor, "Memory compactor initialized", "INFO");
    return MC_SUCCESS;
}

// Allocate memory
void* mc_allocate(MemoryCompactor* compactor, size_t size) {
    pthread_mutex_lock(&compactor->compactor_mutex);
    
    for (int i = 0; i < compactor->block_count; i++) {
        pthread_mutex_lock(&compactor->blocks[i].block_mutex);
        
        if (compactor->blocks[i].is_free && compactor->blocks[i].size >= size) {
            // Split block if necessary
            if (compactor->blocks[i].size > size + sizeof(MemoryBlock)) {
                // Create new block from remaining space
                memmove(&compactor->blocks[compactor->block_count],
                        &compactor->blocks[i],
                        sizeof(MemoryBlock));
                
                compactor->blocks[compactor->block_count].start = 
                    compactor->blocks[i].start + size;
                compactor->blocks[compactor->block_count].size = 
                    compactor->blocks[i].size - size;
                compactor->blocks[compactor->block_count].is_free = true;
                pthread_mutex_init(&compactor->blocks[compactor->block_count].block_mutex, NULL);
                
                compactor->block_count++;
                compactor->blocks[i].size = size;
            }
            
            compactor->blocks[i].is_free = false;
            void* allocated = compactor->blocks[i].start;
            
            pthread_mutex_unlock(&compactor->blocks[i].block_mutex);
            pthread_mutex_unlock(&compactor->compactor_mutex);
            
            char log_msg[MAX_LOG_MESSAGE];
            snprintf(log_msg, MAX_LOG_MESSAGE, "Allocated %zu bytes", size);
            log_message(compactor, log_msg, "INFO");
            
            return allocated;
        }
        
        pthread_mutex_unlock(&compactor->blocks[i].block_mutex);
    }
    
    pthread_mutex_unlock(&compactor->compactor_mutex);
    log_message(compactor, "Allocation failed - no suitable block found", "ERROR");
    return NULL;
}

// Free memory
MemoryCompactorError mc_free(MemoryCompactor* compactor, void* ptr) {
    if (!ptr) return MC_ERROR_INVALID_PARAMETER;
    
    pthread_mutex_lock(&compactor->compactor_mutex);
    
    for (int i = 0; i < compactor->block_count; i++) {
        pthread_mutex_lock(&compactor->blocks[i].block_mutex);
        
        if (compactor->blocks[i].start == ptr) {
            compactor->blocks[i].is_free = true;
            pthread_mutex_unlock(&compactor->blocks[i].block_mutex);
            
            // Attempt to merge adjacent free blocks
            compact_memory(compactor);
            
            pthread_mutex_unlock(&compactor->compactor_mutex);
            
            char log_msg[MAX_LOG_MESSAGE];
            snprintf(log_msg, MAX_LOG_MESSAGE, "Freed memory at %p", ptr);
            log_message(compactor, log_msg, "INFO");
            
            return MC_SUCCESS;
        }
        
        pthread_mutex_unlock(&compactor->blocks[i].block_mutex);
    }
    
    pthread_mutex_unlock(&compactor->compactor_mutex);
    log_message(compactor, "Invalid pointer passed to mc_free", "ERROR");
    return MC_ERROR_INVALID_PARAMETER;
}

// Compact memory
static MemoryCompactorError compact_memory(MemoryCompactor* compactor) {
    bool compacted = false;
    
    do {
        compacted = false;
        
        for (int i = 0; i < compactor->block_count - 1; i++) {
            pthread_mutex_lock(&compactor->blocks[i].block_mutex);
            pthread_mutex_lock(&compactor->blocks[i + 1].block_mutex);
            
            if (compactor->blocks[i].is_free && compactor->blocks[i + 1].is_free) {
                // Merge blocks
                compactor->blocks[i].size += compactor->blocks[i + 1].size;
                
                // Remove the second block
                memmove(&compactor->blocks[i + 1],
                        &compactor->blocks[i + 2],
                        sizeof(MemoryBlock) * (compactor->block_count - i - 2));
                
                compactor->block_count--;
                compacted = true;
                
                pthread_mutex_unlock(&compactor->blocks[i + 1].block_mutex);
                pthread_mutex_unlock(&compactor->blocks[i].block_mutex);
                
                log_message(compactor, "Merged adjacent free blocks", "INFO");
                break;
            }
            
            pthread_mutex_unlock(&compactor->blocks[i + 1].block_mutex);
            pthread_mutex_unlock(&compactor->blocks[i].block_mutex);
        }
    } while (compacted);
    
    return MC_SUCCESS;
}

// Get fragmentation statistics
typedef struct {
    size_t total_free;
    size_t largest_free;
    int free_block_count;
    double fragmentation_ratio;
} FragmentationStats;

FragmentationStats get_fragmentation_stats(MemoryCompactor* compactor) {
    FragmentationStats stats = {0};
    
    pthread_mutex_lock(&compactor->compactor_mutex);
    
    for (int i = 0; i < compactor->block_count; i++) {
        pthread_mutex_lock(&compactor->blocks[i].block_mutex);
        
        if (compactor->blocks[i].is_free) {
            stats.total_free += compactor->blocks[i].size;
            stats.free_block_count++;
            
            if (compactor->blocks[i].size > stats.largest_free) {
                stats.largest_free = compactor->blocks[i].size;
            }
        }
        
        pthread_mutex_unlock(&compactor->blocks[i].block_mutex);
    }
    
    stats.fragmentation_ratio = 
        (double)(stats.free_block_count) / (stats.total_free > 0 ? stats.total_free : 1);
    
    pthread_mutex_unlock(&compactor->compactor_mutex);
    
    char log_msg[MAX_LOG_MESSAGE];
    snprintf(log_msg, MAX_LOG_MESSAGE, 
             "Fragmentation stats - Total free: %zu, Largest free: %zu, Free blocks: %d",
             stats.total_free, stats.largest_free, stats.free_block_count);
    log_message(compactor, log_msg, "INFO");
    
    return stats;
}

// Cleanup
void cleanup_compactor(MemoryCompactor* compactor) {
    if (!compactor) return;
    
    // Clean up mutexes
    pthread_mutex_lock(&compactor->compactor_mutex);
    for (int i = 0; i < compactor->block_count; i++) {
        pthread_mutex_destroy(&compactor->blocks[i].block_mutex);
    }
    
    // Free memory pool
    free(compactor->memory_pool);
    
    // Close log file
    if (compactor->log_file) {
        fclose(compactor->log_file);
    }
    
    pthread_mutex_unlock(&compactor->compactor_mutex);
    pthread_mutex_destroy(&compactor->compactor_mutex);
    
    log_message(compactor, "Memory compactor cleaned up", "INFO");
}