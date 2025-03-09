#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>

// Konfiguration für große Seiten
#define LARGE_PAGE_SIZE (2 * 1024 * 1024) // 2 MB große Seiten
define ALIGN_UP(x, align) (((x) + (align)-1) & ~((align)-1))

// Struktur zur Verwaltung von Speicherblöcken
typedef struct block {
    size_t size;
    struct block* next;
} block_t;

// Globaler Zeiger für die Verwaltung
static block_t* free_list = NULL;

// Funktion für benutzerdefinierte mmap
void* custom_mmap(size_t size) {
    void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (addr == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }
    return addr;
}

// NUMA-aware Speicherzuteilung
void* numa_alloc(size_t size, int node) {
    void* addr = custom_mmap(size);
    if (!addr) {
        return NULL;
    }

    // Bindung an NUMA-Knoten
    unsigned long nodemask = 1UL << node;
    if (mbind(addr, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, MPOL_MF_STRICT) != 0) {
        perror("mbind failed");
        munmap(addr, size);
        return NULL;
    }
    return addr;
}

// Custom malloc
void* custom_malloc(size_t size) {
    size = ALIGN_UP(size + sizeof(block_t), LARGE_PAGE_SIZE);

    block_t* block = free_list;
    block_t** prev_next = &free_list;

    // Suche nach einem passenden Block
    while (block) {
        if (block->size >= size) {
            *prev_next = block->next;
            return (void*)(block + 1);
        }
        prev_next = &block->next;
        block = block->next;
    }

    // Neuer Block, wenn kein passender gefunden wurde
    block = (block_t*)numa_alloc(size, 0); // Standardmäßig NUMA-Knoten 0
    if (!block) {
        return NULL;
    }

    block->size = size;
    return (void*)(block + 1);
}

// Custom free
void custom_free(void* ptr) {
    if (!ptr) return;

    block_t* block = (block_t*)ptr - 1;
    block->next = free_list;
    free_list = block;
}

// Testfall
int main() {
    if (numa_available() < 0) {
        fprintf(stderr, "NUMA is not available on this system\n");
        return 1;
    }

    void* ptr = custom_malloc(1024 * 1024); // 1 MB
    if (ptr) {
        printf("Memory allocated at %p\n", ptr);
        custom_free(ptr);
        printf("Memory freed\n");
    } else {
        printf("Allocation failed\n");
    }

    return 0;
}
