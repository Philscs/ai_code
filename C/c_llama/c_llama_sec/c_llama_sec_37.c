#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

// Struktur für den Speicherplatzbestand
typedef struct {
    size_t total;
    size_t free;
    size_t allocated;
} MemoryPool;

// Struktur für den Log-File
typedef struct {
    FILE* file;
    char filename[256];
} Logger;

// Globale Variablen
Logger logger;

// Funktion um den Speicherpool zu initialisieren
void memory_compactor_init(MemoryPool* pool) {
    // Erstelle einen neuen Speicherpool mit einem festen Größe
    pool->total = 1024 * 1024;  // 1 MB
    pool->free = pool->total;
    pool->allocated = 0;

    // Erstelle ein Log-File, um Fehler und Informationen aufzubewahren
    sprintf(logger.filename, "memory_compactor_log.txt");
    logger.file = fopen(logger.filename, "w+");
    if (!logger.file) {
        fprintf(stderr, "Fehler beim Öffnen des Logs: %s\n", strerror(errno));
        exit(1);
    }
}

// Funktion um einen Speicherplatz zu reservieren
size_t memory_compactor_alloc(MemoryPool* pool, size_t size) {
    // Überprüfe ob genügend Speicherplatz verfügbar ist
    if (pool->free < size) {
        return -1;  // Fehler: nicht genügend Speicherplatz
    }

    // Reserviere den gewünschten Speicherplatz
    void* ptr = malloc(size);
    pool->allocated += size;
    pool->free -= size;

    // Log das Vorkommen eines neuen Speicherplatzes
    fprintf(logger.file, "Allocated %zu bytes\n", size);

    return (size_t)ptr;
}

// Funktion um einen bereits reservierten Speicherplatz zu freigen
void memory_compactor_free(MemoryPool* pool, void* ptr) {
    // Überprüfe ob der gewünschte Speicherplatz gefunden wird
    if (!ptr) {
        printf("Kein Speicherplatz gefunden\n");
        return;
    }

    free(ptr);
    pool->allocated -= 1;
    pool->free += 1;

    // Log das Vorkommen eines freigenlegten Speicherplatzes
    fprintf(logger.file, "Freed %zu bytes\n", 1);
}

// Funktion um den Speicherpool zu komprimieren
void memory_compactor_compress(MemoryPool* pool) {
    size_t free_space = pool->free;
    if (free_space == 0) {
        printf("Keine freien Speicherplätze verfügbar\n");
        return;
    }

    // Log das Vorkommen eines komprimierten Speicherplatzes
    fprintf(logger.file, "Compacted %zu bytes\n", free_space);

    pool->total -= free_space;
}

// Funktion zum Testen der Implementierung
void test() {
    MemoryPool pool;
    memory_compactor_init(&pool);

    // Reserviere 10 Byte und logge das Vorkommen eines neuen Speicherplatzes
    void* ptr = memory_compactor_alloc(&pool, 10);
    if (ptr) {
        printf("Speicherplatz erfolgreich reserviert\n");
        memory_compactor_log(&logger, "Allocated 10 bytes");
    } else {
        printf("Fehler beim Reservieren des Speicherplatzes: nicht genügend Speicherplatz\n");
    }

    // Freigebe den gewünschten Speicherplatz und logge das Vorkommen eines freigenlegten Speicherplatzes
    memory_compactor_free(&pool, ptr);
    if (ptr) {
        printf("Speicherplatz erfolgreich freigegeben\n");
        memory_compactor_log(&logger, "Freed 10 bytes");
    } else {
        printf("Kein Speicherplatz gefunden\n");
    }

    // Komprimiere den Speicherpool und logge das Vorkommen eines komprimierten Speicherplatzes
    memory_compactor_compress(&pool);
}

int main() {
    test();
    return 0;
}
