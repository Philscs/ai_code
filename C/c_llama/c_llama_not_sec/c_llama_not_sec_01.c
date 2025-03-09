#include <stdio.h>
#include <stdlib.h>

// Struktur für eine Prozess-Variable
typedef struct Process {
    int pid;
    int arrivalTime;
    int burstTime;
    int priority;
    struct Process* next;
} Process;

// Struktur für den Heap
typedef struct Heap {
    Process* heap[1000];
    int size;
} Heap;

// Funktion, um ein neues Element dem Heap hinzu zu fügen
void insert(Heap* h, Process* p) {
    // Finden das geeignete Platz in der Liste
    for (int i = 0; i < h->size; i++) {
        if (p->arrivalTime <= h->heap[i]->arrivalTime) {
            // Platz nach dem Element einfügen
            h->heap[h->size++] = p;
            return;
        }
    }

    // Wenn alle Platz in der Liste erfasst sind, sollten wir auf einen Full-Scheduler umgehen
    printf("Heap ist voll\n");
}

// Funktion, um das Element mit der höchsten Priorität aus dem Heap zu entfernen
void remove(Heap* h) {
    Process* min = h->heap[0];
    // Finden die erste Position im Heap, in der ein Wert gefunden wird
    for (int i = 1; i < h->size; i++) {
        if (min->priority > h->heap[i]->priority) {
            min = h->heap[i];
        }
    }

    // Wenn eine Prozess-Struktur entfernt werden soll, sollten wir sicherstellen, dass diese Struktur tatsächlich in der Liste vorkommt
    printf("Entfernen von %d\n", min->pid);
}

int main() {
    Heap heap;
    heap.size = 0;

    // Erzeugen ein paar Prozesse
    Process p1 = {1, 0, 10, 3};
    Process p2 = {2, 5, 7, 1};
    Process p3 = {3, 8, 4, 2};

    // Fügen die Prozesse dem Heap hinzu
    insert(&heap, &p1);
    insert(&heap, &p2);
    insert(&heap, &p3);

    // Entfernen des Elements mit der höchsten Priorität
    remove(&heap);

    return 0;
}