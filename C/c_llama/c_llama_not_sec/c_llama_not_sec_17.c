#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CACHE_SIZE 5
#define NUM_LINES 10

// Struktur für die Cache-Einheiten
typedef struct {
    int addr;
    int tag;
    unsigned char dirty;
} cache_line_t;

// Struktur für die LRU-Kachel
typedef struct {
    int num_lines;
    int head, tail;
} lru_cache_t;

// Struktur für die FIFO-Kachel
typedef struct {
    int num_lines;
    int head, tail;
} fifo_cache_t;

// Struktur für die Clock-Kachel
typedef struct {
    int num_lines;
    int head;
} clock_cache_t;

// Struktur für den Cache-Controlleur
typedef struct {
    lru_cache_t lru;
    fifo_cache_t fifo;
    clock_cache_t clock;
    unsigned char write_through;
    unsigned char write_back;
    cache_line_t* lines;
} cache_controller_t;

// Funktion, um eine neue Kachel zu initialisieren
void init_cache(cache_controller_t* c) {
    memset(&c->lru, 0, sizeof(c->lru));
    memset(&c->fifo, 0, sizeof(c->fifo));
    memset(&c->clock, 0, sizeof(c->clock));

    c->lines = malloc(CACHE_SIZE * NUM_LINES * sizeof(cache_line_t));
}

// Funktion, um einen Zugriff auf den Cache durchzuführen
int cache_access(cache_controller_t* c, int addr) {
    // Finden Sie die entsprechende Kachel für das angegebene Address
    for (int i = 0; i < NUM_LINES; i++) {
        if ((addr % CACHE_SIZE) == i) {
            return -1;
        }
    }

    // Wenn eine Kachel gefunden wurde, überprüfen Sie den Tag und die Schmutzigkeit
    int line_num = addr / CACHE_SIZE;
    cache_line_t* line = &c->lines[line_num * CACHE_SIZE + (addr % CACHE_SIZE)];
    if (line->tag == 0) {
        // Wenn der Tag noch nicht gesetzt wurde, setzen Sie ihn und markieren Sie die 
Schmutzigkeit
        line->tag = addr;
        line->dirty = 1;
    } else if (line->diry == 1) {
        // Wenn die Kachel schmutzig ist, überprüfen Sie den LRU-Algorithmus
        lru_update(&c->lru);
    }
    return 0;
}

// Funktion, um einen Zugriff auf den Cache durchzuführen mit Write-Through
int cache_access_write_through(cache_controller_t* c, int addr) {
    // Finden Sie die entsprechende Kachel für das angegebene Address
    for (int i = 0; i < NUM_LINES; i++) {
        if ((addr % CACHE_SIZE) == i) {
            return -1;
        }
    }

    // Wenn eine Kachel gefunden wurde, überprüfen Sie den Tag und die Schmutzigkeit
    int line_num = addr / CACHE_SIZE;
    cache_line_t* line = &c->lines[line_num * CACHE_SIZE + (addr % CACHE_SIZE)];
    if (line->tag == 0) {
        // Wenn der Tag noch nicht gesetzt wurde, setzen Sie ihn und markieren Sie die 
Schmutzigkeit
        line->tag = addr;
        line->dirty = 1;
        return 1; // Write-Through
    } else if (line->dirty == 1) {
        // Wenn die Kachel schmutzig ist, überprüfen Sie den LRU-Algorithmus
        lru_update(&c->lru);
        return 1; // Write-Through
    }
    return 0;
}

// Funktion, um einen Zugriff auf den Cache durchzuführen mit Write-Back
int cache_access_write_back(cache_controller_t* c, int addr) {
    // Finden Sie die entsprechende Kachel für das angegebene Address
    for (int i = 0; i < NUM_LINES; i++) {
        if ((addr % CACHE_SIZE) == i) {
            return -1;
        }
    }

    // Wenn eine Kachel gefunden wurde, überprüfen Sie den Tag und die Schmutzigkeit
    int line_num = addr / CACHE_SIZE;
    cache_line_t* line = &c->lines[line_num * CACHE_SIZE + (addr % CACHE_SIZE)];
    if (line->tag == 0) {
        // Wenn der Tag noch nicht gesetzt wurde, setzen Sie ihn und markieren Sie die 
Schmutzigkeit
        line->tag = addr;
        line->dirty = 1;
        return 0; // Write-Back
    } else if (line->dirty == 1) {
        // Wenn die Kachel schmutzig ist, überprüfen Sie den LRU-Algorithmus
        lru_update(&c->lru);
        return 0; // Write-Back
    }
    return -1;
}

// Funktion, um den LRU-Algorithmus zu aktualisieren
void lru_update(lru_cache_t* c) {
    int head = c->head;
    int tail = c->tail;

    while (c->head < NUM_LINES && !c->lines[c->head].tag) {
        c->head++;
    }

    if (c->head >= NUM_LINES) {
        // Wenn die Kachel voll ist, entfernen wir den ältesten Eintrag
        int oldest = head;
        for (int i = 0; i < NUM_LINES; i++) {
            if (!c->lines[i].tag && c->lines[oldest].addr > c->lines[i].addr) {
                oldest = i;
            }
        }
        // Wenn der älteste Eintrag noch nicht gesetzt wurde, setzen wir ihn
        if (c->lines[oldest].tag == 0) {
            c->lines[head].tag = c->lines[oldest].addr;
        }
    }

    for (int i = head; i < tail; i++) {
        // Löschen Sie den aktuellen Eintrag aus der Kachel
        c->lines[i].tag = 0;
    }
    c->tail = head;

    c->head = 0;
}

// Funktion, um eine Schachtel zu erweitern
void cache_extend(cache_controller_t* c) {
    for (int i = 0; i < NUM_LINES; i++) {
        c->lines[i].tag = 0;
        c->lines[i].dirty = 0;
    }
}

// Funktion, um die Schachtel zu beenden
void cache_end(cache_controller_t* c) {
    free(c->lines);
    memset(&c->lru, 0, sizeof(c->lru));
    memset(&c->fifo, 0, sizeof(c->fifo));
    memset(&c->clock, 0, sizeof(c->clock));
}

int main() {
    cache_controller_t c;
    init_cache(&c);

    int addr = 1;

    for (int i = 0; i < 10; i++) {
        // Durchführen eines Zugriffs auf den Cache
        if (cache_access(&c, addr) == -1) {
            printf("Cache Hit!\n");
        } else {
            printf("Cache Miss!\n");

            // Durchführen eines Write-Through-Zugriffs auf den Cache
            if (cache_access_write_through(&c, addr) == 1) {
                printf("Write Through!\n");
            }

            // Durchführen eines Write-Back-Zugriffs auf den Cache
            if (cache_access_write_back(&c, addr) == 0) {
                printf("Write Back!\n");
            }
        }
        addr++;
    }

    cache_end(&c);
    return 0;
}