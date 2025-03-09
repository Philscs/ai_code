#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>

// Struktur für den Speicherblock-Header
typedef struct block_header {
    size_t size;                 // Größe des Blocks
    int numa_node;              // NUMA Node ID
    int is_huge_page;           // Huge Page Flag
    struct block_header *next;   // Nächster freier Block
    int is_free;                // Block-Status
} block_header_t;

// Globale Variablen
static block_header_t *free_list = NULL;
static const size_t HUGE_PAGE_SIZE = 2 * 1024 * 1024; // 2MB Huge Pages
static const size_t MIN_ALLOC_SIZE = sizeof(block_header_t);

// Initialisierung des Speichermanagers
void init_memory_manager(void) {
    if (numa_available() == -1) {
        fprintf(stderr, "NUMA nicht verfügbar\n");
        exit(1);
    }
}

// Hilfsfunktion zur Bestimmung ob Huge Pages verwendet werden sollen
static int should_use_huge_pages(size_t size) {
    return size >= HUGE_PAGE_SIZE / 2;
}

// Hilfsfunktion zur Ermittlung des optimalen NUMA Nodes
static int get_optimal_numa_node(void) {
    int cpu = sched_getcpu();
    return numa_node_of_cpu(cpu);
}

// Speicher von einem bestimmten NUMA Node allozieren
static void* numa_alloc_memory(size_t size, int node, int use_huge_pages) {
    void* ptr;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    
    if (use_huge_pages) {
        flags |= MAP_HUGETLB;
    }
    
    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    
    // Speicher an NUMA Node binden
    unsigned long node_mask = 1UL << node;
    if (mbind(ptr, size, MPOL_PREFERRED, &node_mask, sizeof(node_mask) * 8, 0) == -1) {
        munmap(ptr, size);
        return NULL;
    }
    
    return ptr;
}

// Hauptallokationsfunktion
void* numa_malloc(size_t size) {
    if (size == 0) return NULL;
    
    // Aufrunden auf Vielfaches von 8 Bytes für Alignment
    size = (size + 7) & ~7;
    size_t total_size = size + sizeof(block_header_t);
    
    // Prüfen ob Huge Pages verwendet werden sollen
    int use_huge_pages = should_use_huge_pages(total_size);
    if (use_huge_pages) {
        total_size = (total_size + HUGE_PAGE_SIZE - 1) & ~(HUGE_PAGE_SIZE - 1);
    }
    
    // Optimalen NUMA Node ermitteln
    int node = get_optimal_numa_node();
    
    // Speicher allozieren
    void* memory = numa_alloc_memory(total_size, node, use_huge_pages);
    if (!memory) return NULL;
    
    // Block-Header initialisieren
    block_header_t* header = (block_header_t*)memory;
    header->size = total_size - sizeof(block_header_t);
    header->numa_node = node;
    header->is_huge_page = use_huge_pages;
    header->is_free = 0;
    header->next = NULL;
    
    // Wenn der Block größer ist als angefordert, den Rest freigeben
    if (header->size > size + MIN_ALLOC_SIZE) {
        block_header_t* new_block = (block_header_t*)((char*)memory + sizeof(block_header_t) + size);
        new_block->size = header->size - size - sizeof(block_header_t);
        new_block->numa_node = node;
        new_block->is_huge_page = use_huge_pages;
        new_block->is_free = 1;
        new_block->next = free_list;
        
        header->size = size;
        free_list = new_block;
    }
    
    return (char*)memory + sizeof(block_header_t);
}

// Speicher freigeben
void numa_free(void* ptr) {
    if (!ptr) return;
    
    block_header_t* header = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    
    // Prüfen ob der nächste Block frei ist und ggf. zusammenführen
    block_header_t* current = free_list;
    block_header_t* prev = NULL;
    
    while (current) {
        if ((char*)header + sizeof(block_header_t) + header->size == (char*)current) {
            header->size += sizeof(block_header_t) + current->size;
            header->next = current->next;
            if (prev) {
                prev->next = header;
            } else {
                free_list = header;
            }
            break;
        }
        prev = current;
        current = current->next;
    }
    
    // Wenn Huge Pages verwendet wurden, gesamten Speicherbereich freigeben
    if (header->is_huge_page) {
        size_t total_size = sizeof(block_header_t) + header->size;
        total_size = (total_size + HUGE_PAGE_SIZE - 1) & ~(HUGE_PAGE_SIZE - 1);
        munmap(header, total_size);
    } else {
        // Sonst Block als frei markieren und in die Free-Liste einfügen
        header->is_free = 1;
        header->next = free_list;
        free_list = header;
    }
}

// Speicher reallozieren
void* numa_realloc(void* ptr, size_t new_size) {
    if (!ptr) return numa_malloc(new_size);
    if (new_size == 0) {
        numa_free(ptr);
        return NULL;
    }
    
    block_header_t* header = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    
    // Wenn neuer Speicher kleiner ist, Block eventuell aufteilen
    if (new_size <= header->size) {
        if (header->size > new_size + MIN_ALLOC_SIZE) {
            block_header_t* new_block = (block_header_t*)((char*)ptr + new_size);
            new_block->size = header->size - new_size - sizeof(block_header_t);
            new_block->numa_node = header->numa_node;
            new_block->is_huge_page = header->is_huge_page;
            new_block->is_free = 1;
            new_block->next = free_list;
            
            header->size = new_size;
            free_list = new_block;
        }
        return ptr;
    }
    
    // Neuen Speicher allozieren und Daten kopieren
    void* new_ptr = numa_malloc(new_size);
    if (!new_ptr) return NULL;
    
    memcpy(new_ptr, ptr, header->size);
    numa_free(ptr);
    
    return new_ptr;
}