#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MEMORY_POOL_SIZE (1024 * 1024)  // 1MB Speicherpool
#define MAX_ALLOCATIONS 1024
#define CANARY_VALUE 0xDEADBEEF

// Strukturen für den Speichermanager
typedef struct MemoryBlock {
    void* start;
    size_t size;
    unsigned int ref_count;
    bool is_free;
    unsigned int canary;
    struct MemoryBlock* next;
    struct MemoryBlock* prev;
} MemoryBlock;

typedef struct {
    void* pointer;
    MemoryBlock* block;
} AllocationEntry;

typedef struct {
    void* memory_pool;
    MemoryBlock* first_block;
    AllocationEntry allocations[MAX_ALLOCATIONS];
    size_t allocation_count;
    bool gc_enabled;
} MemoryManager;

// Globaler Speichermanager
static MemoryManager* g_manager = NULL;

// Initialisierung des Speichermanagers
MemoryManager* init_memory_manager() {
    MemoryManager* manager = (MemoryManager*)malloc(sizeof(MemoryManager));
    if (!manager) return NULL;

    manager->memory_pool = malloc(MEMORY_POOL_SIZE);
    if (!manager->memory_pool) {
        free(manager);
        return NULL;
    }

    // Initialisierung des ersten freien Blocks
    manager->first_block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    manager->first_block->start = manager->memory_pool;
    manager->first_block->size = MEMORY_POOL_SIZE;
    manager->first_block->ref_count = 0;
    manager->first_block->is_free = true;
    manager->first_block->canary = CANARY_VALUE;
    manager->first_block->next = NULL;
    manager->first_block->prev = NULL;

    manager->allocation_count = 0;
    manager->gc_enabled = true;

    g_manager = manager;
    return manager;
}

// Referenzzählung
void increment_ref_count(void* ptr) {
    for (size_t i = 0; i < g_manager->allocation_count; i++) {
        if (g_manager->allocations[i].pointer == ptr) {
            g_manager->allocations[i].block->ref_count++;
            break;
        }
    }
}

void decrement_ref_count(void* ptr) {
    for (size_t i = 0; i < g_manager->allocation_count; i++) {
        if (g_manager->allocations[i].pointer == ptr) {
            g_manager->allocations[i].block->ref_count--;
            if (g_manager->allocations[i].block->ref_count == 0) {
                // Markiere Block als frei für GC
                g_manager->allocations[i].block->is_free = true;
            }
            break;
        }
    }
}

// Zyklische Referenzerkennung
typedef struct {
    void* ptr;
    bool marked;
} CycleDetectionEntry;

void detect_cycles() {
    CycleDetectionEntry* entries = malloc(sizeof(CycleDetectionEntry) * g_manager->allocation_count);
    if (!entries) return;

    // Markiere alle Einträge als unbesucht
    for (size_t i = 0; i < g_manager->allocation_count; i++) {
        entries[i].ptr = g_manager->allocations[i].pointer;
        entries[i].marked = false;
    }

    // Implementierung des Mark-and-Sweep Algorithmus
    // Markierungsphase
    for (size_t i = 0; i < g_manager->allocation_count; i++) {
        if (!g_manager->allocations[i].block->is_free && 
            g_manager->allocations[i].block->ref_count > 0) {
            entries[i].marked = true;
        }
    }

    // Aufräumphase
    for (size_t i = 0; i < g_manager->allocation_count; i++) {
        if (!entries[i].marked) {
            // Potentieller Zyklus gefunden
            decrement_ref_count(entries[i].ptr);
        }
    }

    free(entries);
}

// Defragmentierung
void defragment() {
    MemoryBlock* current = g_manager->first_block;
    
    while (current && current->next) {
        if (current->is_free && current->next->is_free) {
            // Kombiniere benachbarte freie Blöcke
            current->size += current->next->size;
            current->next = current->next->next;
            if (current->next) {
                current->next->prev = current;
            }
        } else {
            current = current->next;
        }
    }
}

// Memory Leak Detection
void check_memory_leaks() {
    size_t leaked_bytes = 0;
    size_t leak_count = 0;

    for (size_t i = 0; i < g_manager->allocation_count; i++) {
        if (!g_manager->allocations[i].block->is_free && 
            g_manager->allocations[i].block->ref_count > 0) {
            leaked_bytes += g_manager->allocations[i].block->size;
            leak_count++;
            printf("Memory leak detected at %p, size: %zu bytes\n",
                   g_manager->allocations[i].pointer,
                   g_manager->allocations[i].block->size);
        }
    }

    if (leak_count > 0) {
        printf("Total memory leaks: %zu, Total leaked bytes: %zu\n",
               leak_count, leaked_bytes);
    }
}

// Use-After-Free Prevention
bool validate_pointer(void* ptr) {
    if (!ptr) return false;

    for (size_t i = 0; i < g_manager->allocation_count; i++) {
        if (g_manager->allocations[i].pointer == ptr) {
            MemoryBlock* block = g_manager->allocations[i].block;
            
            // Überprüfe Canary Value
            if (block->canary != CANARY_VALUE) {
                printf("Memory corruption detected at %p\n", ptr);
                return false;
            }

            // Überprüfe, ob der Block frei ist
            if (block->is_free) {
                printf("Use-after-free detected at %p\n", ptr);
                return false;
            }

            return true;
        }
    }

    printf("Invalid pointer access at %p\n", ptr);
    return false;
}

// Speicherallokation
void* secure_malloc(size_t size) {
    if (!g_manager || !size) return NULL;

    // Suche nach einem passenden freien Block
    MemoryBlock* current = g_manager->first_block;
    while (current) {
        if (current->is_free && current->size >= size) {
            // Teile den Block auf, wenn er zu groß ist
            if (current->size > size + sizeof(MemoryBlock) + 32) {
                MemoryBlock* new_block = (MemoryBlock*)((char*)current->start + size);
                new_block->start = (char*)new_block + sizeof(MemoryBlock);
                new_block->size = current->size - size - sizeof(MemoryBlock);
                new_block->ref_count = 0;
                new_block->is_free = true;
                new_block->canary = CANARY_VALUE;
                new_block->next = current->next;
                new_block->prev = current;
                
                current->size = size;
                current->next = new_block;
            }

            current->is_free = false;
            current->ref_count = 1;

            // Registriere die Allokation
            if (g_manager->allocation_count < MAX_ALLOCATIONS) {
                g_manager->allocations[g_manager->allocation_count].pointer = current->start;
                g_manager->allocations[g_manager->allocation_count].block = current;
                g_manager->allocation_count++;
            }

            return current->start;
        }
        current = current->next;
    }

    // Keine passende Lücke gefunden
    defragment();  // Versuche zu defragmentieren
    
    // Versuche erneut nach Defragmentierung
    current = g_manager->first_block;
    while (current) {
        if (current->is_free && current->size >= size) {
            current->is_free = false;
            current->ref_count = 1;
            return current->start;
        }
        current = current->next;
    }

    return NULL;  // Kein Speicher verfügbar
}

// Speicherfreigabe
void secure_free(void* ptr) {
    if (!ptr || !g_manager) return;

    if (!validate_pointer(ptr)) {
        printf("Attempt to free invalid pointer %p\n", ptr);
        return;
    }

    for (size_t i = 0; i < g_manager->allocation_count; i++) {
        if (g_manager->allocations[i].pointer == ptr) {
            decrement_ref_count(ptr);
            
            // Wenn der Referenzzähler 0 erreicht, markiere als frei
            if (g_manager->allocations[i].block->ref_count == 0) {
                g_manager->allocations[i].block->is_free = true;
                
                // Lösche sensitive Daten
                memset(ptr, 0, g_manager->allocations[i].block->size);
                
                // Defragmentiere wenn nötig
                if (g_manager->gc_enabled) {
                    defragment();
                }
            }
            
            break;
        }
    }
}

// Cleanup des Memory Managers
void cleanup_memory_manager() {
    if (!g_manager) return;

    check_memory_leaks();  // Final leak check

    // Bereinige alle Blöcke
    MemoryBlock* current = g_manager->first_block;
    while (current) {
        MemoryBlock* next = current->next;
        free(current);
        current = next;
    }

    // Gebe den Speicherpool frei
    free(g_manager->memory_pool);
    free(g_manager);
    g_manager = NULL;
}

// Beispielnutzung
int main() {
    // Initialisiere Memory Manager
    MemoryManager* manager = init_memory_manager();
    if (!manager) {
        printf("Failed to initialize memory manager\n");
        return 1;
    }

    // Beispiel-Allokationen
    int* numbers = (int*)secure_malloc(5 * sizeof(int));
    if (numbers) {
        for (int i = 0; i < 5; i++) {
            numbers[i] = i;
        }
        
        // Erhöhe Referenzzähler
        increment_ref_count(numbers);
        
        // Überprüfe auf Use-After-Free
        if (validate_pointer(numbers)) {
            printf("Pointer is valid\n");
        }
        
        // Gebe Speicher frei
        secure_free(numbers);
    }

    // Führe Garbage Collection durch
    detect_cycles();
    
    // Überprüfe auf Memory Leaks
    check_memory_leaks();
    
    // Cleanup
    cleanup_memory_manager();
    
    return 0;
}