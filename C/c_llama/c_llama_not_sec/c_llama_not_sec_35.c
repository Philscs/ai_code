#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/sysconf.h>

// Funktion um die Anzahl der Physischen Pages zu ermitteln
size_t getPhysicalPages(void) {
    return sysconf(_SC_PHYS_PAGES);
}

// Funktion um die Größe einer Page zu ermitteln
size_t getPageSize(void) {
    return sysconf(_SC_PAGE_SIZE);
}

// Funktion, um eine große Seite in der Memmory zu reservieren
void* allocLargePage(size_t size, int numaNode) {
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED) {
        // An error occurred while mapping the memory region.
        return NULL;
    }

    // Berechnung der Nummer des numa Nodes, an dem die Memmory angelegt wird.
    size_t numaNodeNum = numa_get_node_num();
    if (numaNode != numaNodeNum) {
        return NULL; // Wenn numaNode nicht das gleiche ist wie numaNodeNum, ist es ein Fehler
    }

    return mem;
}

// Funktion, um eine kleine Seite in der Memmory zu reservieren
void* malloc(int size, int numaNode) {

    // Überprüfung ob die gewünschte Größe für large_page unterstützt wird.
    if (size > getPageSize()) {
        printf("Die gewünschte Größe %zu ist größer als die Maximalgröße einer Large-Page (von 
%zu Bytes).", size, pageSize());
        exit(1);
    }

    // Wenn numaNode 0 ist, dann wird die Memmory auf einem bestimmten numa Node angelegt
    if (numaNode == 0) {
        return allocLargePage(size, numaNode);
    } else {
        return mallocInternal(size, numaNode);
    }
}

// Funktion für die internen malloc Call
void* mallocInternal(int size, int numaNode) {

    // Wenn numaNode ist, dann wird die Memmory auf einem bestimmten numa Node angelegt.
    if (numaNode != 0) {
        return allocLargePage(size, numaNode);
    } else {
        void *mem = malloc(size);

        // Anweisung, um sicherzustellen, dass der Speicher auf dem gewünschten numa-Node liegt
        if (sysconf(_SC_NUCLEUS) == 0 && numa_get_node_num() != numaNode) {
            printf("Die Memmory wurde nicht auf dem gewünschten numa Node angelegt. Fehlende 
Informationen zum numa Node");
            exit(1);
        }

        return mem;
    }
}