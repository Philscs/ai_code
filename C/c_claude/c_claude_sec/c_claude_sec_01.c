#include <stdint.h>
#include <stdbool.h>

// Konfigurationskonstanten
#define MIN_BLOCK_SIZE 32
#define MAX_BLOCK_SIZE 4096
#define POOL_SIZE (1024 * 1024)  // 1MB Pool
#define BOUNDARY_MAGIC 0xDEADBEEF
#define ALIGNMENT 8

// Block Header/Footer Struktur
typedef struct {
    uint32_t size;           // Blockgröße inkl. Header/Footer
    uint32_t magic;          // Magic Number für Boundary Check
    bool is_free;            // Freier/Allokierter Status
    struct block_meta* next; // Pointer auf nächsten Block
    uint32_t checksum;       // Checksumme für Double-Free Detection
} block_meta;

// Pool Verwaltungsstruktur
typedef struct {
    uint8_t memory[POOL_SIZE];
    block_meta* free_list;
    uint32_t total_blocks;
    uint32_t free_blocks;
} memory_pool;

static memory_pool pool;

// Initialisierung des Pools
void pool_init(void) {
    block_meta* initial_block = (block_meta*)pool.memory;
    initial_block->size = POOL_SIZE;
    initial_block->magic = BOUNDARY_MAGIC;
    initial_block->is_free = true;
    initial_block->next = NULL;
    initial_block->checksum = 0;
    
    // Footer setzen
    block_meta* footer = (block_meta*)(pool.memory + POOL_SIZE - sizeof(block_meta));
    footer->magic = BOUNDARY_MAGIC;
    
    pool.free_list = initial_block;
    pool.total_blocks = 1;
    pool.free_blocks = 1;
}

// Berechnung der Checksumme für Double-Free Detection
static uint32_t calculate_checksum(block_meta* block) {
    uint32_t sum = 0;
    uint8_t* ptr = (uint8_t*)block;
    for (size_t i = 0; i < sizeof(block_meta) - sizeof(uint32_t); i++) {
        sum += ptr[i];
    }
    return sum;
}

// Boundary Check
static bool check_boundaries(block_meta* block) {
    if (block->magic != BOUNDARY_MAGIC) {
        return false;
    }
    
    block_meta* footer = (block_meta*)((uint8_t*)block + block->size - sizeof(block_meta));
    return footer->magic == BOUNDARY_MAGIC;
}

// Speicherallokation
void* pool_alloc(size_t size) {
    if (size == 0 || size > MAX_BLOCK_SIZE - sizeof(block_meta) * 2) {
        return NULL;
    }
    
    // Aufrunden auf nächstes Vielfaches von ALIGNMENT
    size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    size += sizeof(block_meta) * 2;  // Platz für Header und Footer
    
    block_meta* current = pool.free_list;
    block_meta* best_fit = NULL;
    size_t min_delta = POOL_SIZE;
    
    // Best-Fit Suche
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            size_t delta = current->size - size;
            if (delta < min_delta) {
                min_delta = delta;
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    if (best_fit == NULL) {
        return NULL;  // Kein passender Block gefunden
    }
    
    // Split block if remainder is large enough
    if (min_delta >= MIN_BLOCK_SIZE + sizeof(block_meta) * 2) {
        block_meta* new_block = (block_meta*)((uint8_t*)best_fit + size);
        new_block->size = best_fit->size - size;
        new_block->magic = BOUNDARY_MAGIC;
        new_block->is_free = true;
        new_block->next = best_fit->next;
        new_block->checksum = calculate_checksum(new_block);
        
        best_fit->size = size;
        best_fit->next = new_block;
        
        pool.total_blocks++;
        pool.free_blocks++;
    }
    
    best_fit->is_free = false;
    best_fit->checksum = calculate_checksum(best_fit);
    
    // Footer setzen
    block_meta* footer = (block_meta*)((uint8_t*)best_fit + best_fit->size - sizeof(block_meta));
    footer->magic = BOUNDARY_MAGIC;
    
    pool.free_blocks--;
    
    // Rückgabe des nutzbaren Speicherbereichs
    return (void*)((uint8_t*)best_fit + sizeof(block_meta));
}

// Speicherfreigabe
void pool_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    block_meta* block = (block_meta*)((uint8_t*)ptr - sizeof(block_meta));
    
    // Boundary Check
    if (!check_boundaries(block)) {
        // Buffer Overflow detected
        // Hier könnte ein Error Handler aufgerufen werden
        return;
    }
    
    // Double-Free Check
    uint32_t current_checksum = calculate_checksum(block);
    if (block->is_free || current_checksum != block->checksum) {
        // Double-Free detected
        // Hier könnte ein Error Handler aufgerufen werden
        return;
    }
    
    block->is_free = true;
    block->checksum = calculate_checksum(block);
    pool.free_blocks++;
    
    // Coalescing mit nachfolgendem Block
    block_meta* next = block->next;
    if (next != NULL && next->is_free) {
        block->size += next->size;
        block->next = next->next;
        pool.total_blocks--;
        pool.free_blocks--;
    }
    
    // Coalescing mit vorhergehendem Block
    block_meta* prev = pool.free_list;
    while (prev != NULL && prev->next != block) {
        prev = prev->next;
    }
    
    if (prev != NULL && prev->is_free) {
        prev->size += block->size;
        prev->next = block->next;
        prev->checksum = calculate_checksum(prev);
        pool.total_blocks--;
        pool.free_blocks--;
    }
}

// Debugging-Funktion zur Anzeige der Pool-Statistiken
void pool_stats(void) {
    printf("Pool Statistics:\n");
    printf("Total Blocks: %u\n", pool.total_blocks);
    printf("Free Blocks: %u\n", pool.free_blocks);
    printf("Used Blocks: %u\n", pool.total_blocks - pool.free_blocks);
}