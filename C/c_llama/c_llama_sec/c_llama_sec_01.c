#include <stdio.h>
#include <stdlib.h>

// Definieren Sie eine Strukt, die den spezifischen Informationen eines Block enthält
typedef struct {
    void* data;
    size_t size;
    int tag; // 0: Freie, 1: Allokiert
} block_info;

// Speicher-Pool-Allocator-Klasse
class allocator {
private:
    // Speicherpool
    void** pool;
    // Anzahl der Blöcke
    size_t count;

public:
    allocator(size_t initial_capacity = 4 * 1024) {
        // Initialisieren Sie den Speicherpool mit dem angegebenen Kapazität
        pool = (void**)malloc((initial_capacity + SIZE_T_MAX - 1) & ~SIZE_T_MAX);
        if (!pool) {
            printf("Fehler: nicht genügend Speicher\n");
            exit(1);
        }
        count = 0;

        // Erstellen Sie den ersten Block
        block_info* block = (block_info*)malloc(sizeof(block_info));
        block->data = pool[0];
        block->size = initial_capacity;
        block->tag = 0; // Freier Block

        pool[1] = NULL; // Pointer auf keinen Block, um die Kapazität zu speichern
    }

    // Funktion zum Allokieren eines Blocks
    void* allocate(size_t size) {
        if (size < SIZE_T_MAX - 64 && count > 0) { // Überprüfen Sie, ob der Block groß genug ist
            for (int i = 2; pool[i]; ++i); // Suchen Sie nach einem freien Block
            if (!pool[i]) { // Wenn kein freier Block gefunden wurde, führen wir ein 
Fragmentation-Mechanismus durch
                // Erstellen Sie einen neuen freien Block
                block_info* new_block = (block_info*)malloc(sizeof(block_info));
                new_block->size = size;
                pool[++count] = NULL; // Speichern Sie den neuen freien Block

                return (void*)((uintptr_t)pool[count - 1] + SIZE_T_MAX - 2); // Rückgabe des 
neuen Blocks
            }

            block_info* found_block = pool[i];
            block_info* previous_block = (block_info*)((uintptr_t)found_block->data + SIZE_T_MAX 
- 2);
            size_t total_size = found_block->size;
            if ((total_size & ~(SIZE_T_MAX - 1)) != size && (previous_block && 
(previous_block->tag == 0))) {
                // Verwenden Sie einen alten freien Block, um Fragmentation zu vermeiden
                block_info* old_block = pool[--count];
                block_info* new_block = (block_info*)malloc(sizeof(block_info));
                new_block->data = old_block->data;
                new_block->size = size;
                new_block->tag = 0;

                pool[1] = NULL; // Speichern Sie den neuen freien Block
            } else {
                block_info* temp_block = (block_info*)malloc(sizeof(block_info));
                if (found_block == previous_block) { // Wenn der vorherige Block bereits vergeben 
ist
                    block_info* next_block = pool[++count];
                    size_t remaining_size = found_block->size - size;
                    free(found_block);
                    temp_block->data = (uintptr_t)previous_block + SIZE_T_MAX - 2;
                    temp_block->size = remaining_size;

                    // Verwenden Sie einen neuen freien Block, um Fragmentation zu vermeiden
                    block_info* old_block = pool[--count];
                    new_block->data = old_block->data;
                } else {
                    block_info* next_block = pool[++count];
                    temp_block->size = size;
                    if (next_block) { // Überprüfen Sie, ob der nächste Block vergeben ist
                        block_info* next_free_block = pool[++count];
                        free(next_block);
                        new_block->data = (uintptr_t)found_block + SIZE_T_MAX - 2;
                        temp_block->size = found_block->size - size;
                    } else {
                        // Verwenden Sie einen neuen freien Block, um Fragmentation zu vermeiden
                        block_info* old_block = pool[--count];
                        new_block->data = old_block->data;
                    }
                }

                pool[1] = NULL; // Speichern Sie den neuen freien Block

                return (void*)new_block;
            }
        } else {
            printf("Fehler: Ungültige Eingabe\n");
            exit(1);
        }
    }

    // Funktion zum Freigeben eines Blocks
    void deallocate(void* ptr) {
        if (!ptr) { // Überprüfen Sie, ob der Block null ist
            return;
        }

        block_info* found_block = (block_info*)((uintptr_t)ptr - SIZE_T_MAX + 2);
        if (!(found_block->tag == 0)) { // Wenn der Block vergeben ist
            printf("Fehler: Block wurde bereits freigegeben\n");
            exit(1);
        }

        for (int i = count; i >= 1; --i) {
            block_info* previous_block = pool[i - 1];
            if ((previous_block->tag == 0 && found_block->size < SIZE_T_MAX) || 
!(found_block->size & ~(SIZE_T_MAX - 1))) {
                // Verwenden Sie einen alten freien Block, um Fragmentation zu vermeiden
                free(found_block);

                if (previous_block != NULL) {
                    previous_block->size += found_block->size;
                }

                return;
            }
        }

        printf("Fehler: Block konnte nicht gefunden werden\n");
        exit(1);
    }
};

int main() {
    allocator alloc;

    void* ptr = alloc.allocate(32); // Allokieren eines Blocks
    printf("%p\n", ptr);

    alloc.deallocate(ptr); // Freigeben eines Blocks

    return 0;
}