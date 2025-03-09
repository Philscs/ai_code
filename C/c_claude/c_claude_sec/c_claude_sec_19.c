#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

// Konstanten für den Allocator
#define BLOCK_SIZE 64                    // Größe jedes Blocks
#define ALIGNMENT 16                     // Memory Alignment
#define POISON_PATTERN 0xFE              // Pattern für Memory Poisoning
#define BOUNDARY_PATTERN 0xDEADBEEF      // Pattern für Boundary Tags
#define MAX_BLOCKS 1024                  // Maximale Anzahl an Blöcken

// Struktur für Block Metadaten
typedef struct {
    uint32_t boundary_start;             // Boundary Tag am Anfang
    size_t size;                         // Größe des Blocks
    bool is_free;                        // Ist der Block frei?
    uint64_t allocation_id;              // Eindeutige ID für Use-After-Free Detection
    uint32_t boundary_end;               // Boundary Tag am Ende
} BlockHeader;

// Hauptstruktur des Allocators
typedef struct {
    void* memory_pool;                   // Pointer auf den Speicherpool
    size_t total_size;                   // Gesamtgröße des Pools
    size_t used_blocks;                  // Anzahl genutzter Blöcke
    uint64_t next_allocation_id;         // Nächste zu vergebende Allocation ID
    pthread_mutex_t mutex;               // Mutex für Thread Safety
} FixedAllocator;

// Initialisierung des Allocators
FixedAllocator* init_allocator(void) {
    FixedAllocator* allocator = (FixedAllocator*)malloc(sizeof(FixedAllocator));
    if (!allocator) return NULL;

    // Berechne aligned Größe für den gesamten Pool
    size_t aligned_block_size = (BLOCK_SIZE + sizeof(BlockHeader) + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    allocator->total_size = aligned_block_size * MAX_BLOCKS;
    
    // Allocate aligned memory
    if (posix_memalign(&allocator->memory_pool, ALIGNMENT, allocator->total_size) != 0) {
        free(allocator);
        return NULL;
    }

    // Initialisiere Metadaten
    allocator->used_blocks = 0;
    allocator->next_allocation_id = 1;
    pthread_mutex_init(&allocator->mutex, NULL);

    // Initialisiere alle Blöcke als frei
    char* current = (char*)allocator->memory_pool;
    for (size_t i = 0; i < MAX_BLOCKS; i++) {
        BlockHeader* header = (BlockHeader*)current;
        header->boundary_start = BOUNDARY_PATTERN;
        header->size = BLOCK_SIZE;
        header->is_free = true;
        header->allocation_id = 0;
        header->boundary_end = BOUNDARY_PATTERN;
        
        // Poison den Speicherbereich
        memset(current + sizeof(BlockHeader), POISON_PATTERN, BLOCK_SIZE);
        
        current += aligned_block_size;
    }

    return allocator;
}

// Allokation eines Blocks
void* fixed_malloc(FixedAllocator* allocator) {
    if (!allocator) return NULL;
    
    pthread_mutex_lock(&allocator->mutex);
    
    // Suche einen freien Block
    char* current = (char*)allocator->memory_pool;
    size_t aligned_block_size = (BLOCK_SIZE + sizeof(BlockHeader) + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    
    for (size_t i = 0; i < MAX_BLOCKS; i++) {
        BlockHeader* header = (BlockHeader*)current;
        
        // Prüfe Boundary Tags
        if (header->boundary_start != BOUNDARY_PATTERN || 
            header->boundary_end != BOUNDARY_PATTERN) {
            fprintf(stderr, "Memory corruption detected!\n");
            pthread_mutex_unlock(&allocator->mutex);
            return NULL;
        }
        
        if (header->is_free) {
            header->is_free = false;
            header->allocation_id = allocator->next_allocation_id++;
            allocator->used_blocks++;
            
            // Clear poison pattern
            void* user_ptr = current + sizeof(BlockHeader);
            memset(user_ptr, 0, BLOCK_SIZE);
            
            pthread_mutex_unlock(&allocator->mutex);
            return user_ptr;
        }
        
        current += aligned_block_size;
    }
    
    pthread_mutex_unlock(&allocator->mutex);
    return NULL;
}

// Freigabe eines Blocks
void fixed_free(FixedAllocator* allocator, void* ptr) {
    if (!allocator || !ptr) return;
    
    pthread_mutex_lock(&allocator->mutex);
    
    // Finde den Block Header
    char* current = (char*)allocator->memory_pool;
    size_t aligned_block_size = (BLOCK_SIZE + sizeof(BlockHeader) + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    
    for (size_t i = 0; i < MAX_BLOCKS; i++) {
        void* user_ptr = current + sizeof(BlockHeader);
        
        if (user_ptr == ptr) {
            BlockHeader* header = (BlockHeader*)current;
            
            // Prüfe Boundary Tags
            if (header->boundary_start != BOUNDARY_PATTERN || 
                header->boundary_end != BOUNDARY_PATTERN) {
                fprintf(stderr, "Memory corruption detected during free!\n");
                pthread_mutex_unlock(&allocator->mutex);
                return;
            }
            
            // Prüfe ob der Block bereits freigegeben wurde
            if (header->is_free) {
                fprintf(stderr, "Double free detected!\n");
                pthread_mutex_unlock(&allocator->mutex);
                return;
            }
            
            // Markiere als frei und poison den Speicher
            header->is_free = true;
            memset(user_ptr, POISON_PATTERN, BLOCK_SIZE);
            allocator->used_blocks--;
            
            pthread_mutex_unlock(&allocator->mutex);
            return;
        }
        
        current += aligned_block_size;
    }
    
    fprintf(stderr, "Invalid pointer passed to fixed_free!\n");
    pthread_mutex_unlock(&allocator->mutex);
}

// Validierung eines Pointers
bool validate_pointer(FixedAllocator* allocator, void* ptr) {
    if (!allocator || !ptr) return false;
    
    char* current = (char*)allocator->memory_pool;
    size_t aligned_block_size = (BLOCK_SIZE + sizeof(BlockHeader) + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    
    for (size_t i = 0; i < MAX_BLOCKS; i++) {
        void* user_ptr = current + sizeof(BlockHeader);
        
        if (user_ptr == ptr) {
            BlockHeader* header = (BlockHeader*)current;
            return !header->is_free && 
                   header->boundary_start == BOUNDARY_PATTERN && 
                   header->boundary_end == BOUNDARY_PATTERN;
        }
        
        current += aligned_block_size;
    }
    
    return false;
}

// Cleanup des Allocators
void destroy_allocator(FixedAllocator* allocator) {
    if (!allocator) return;
    
    pthread_mutex_destroy(&allocator->mutex);
    free(allocator->memory_pool);
    free(allocator);
}

// Beispiel zur Verwendung
int main() {
    // Initialisiere den Allocator
    FixedAllocator* allocator = init_allocator();
    if (!allocator) {
        fprintf(stderr, "Failed to initialize allocator\n");
        return 1;
    }
    
    // Allokiere einige Blöcke
    void* ptr1 = fixed_malloc(allocator);
    void* ptr2 = fixed_malloc(allocator);
    
    // Validiere die Pointer
    printf("ptr1 valid: %d\n", validate_pointer(allocator, ptr1));
    printf("ptr2 valid: %d\n", validate_pointer(allocator, ptr2));
    
    // Gebe Speicher frei
    fixed_free(allocator, ptr1);
    fixed_free(allocator, ptr2);
    
    // Cleanup
    destroy_allocator(allocator);
    
    return 0;
}