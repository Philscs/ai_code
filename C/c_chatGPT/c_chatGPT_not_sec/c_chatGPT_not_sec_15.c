#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Struktur für Objekte
typedef struct Object {
    bool marked;
    struct Object* next;
    struct Object** references;
    int reference_count;
} Object;

// Root-Set und Heap
typedef struct {
    Object* head;
} RootSet;

RootSet root_set = { NULL };
Object* heap = NULL;

// Hilfsfunktionen
Object* create_object(int reference_count) {
    Object* obj = (Object*)malloc(sizeof(Object));
    obj->marked = false;
    obj->next = NULL;
    obj->reference_count = reference_count;
    if (reference_count > 0) {
        obj->references = (Object**)malloc(sizeof(Object*) * reference_count);
        memset(obj->references, 0, sizeof(Object*) * reference_count);
    } else {
        obj->references = NULL;
    }
    // Objekt zum Heap hinzufügen
    obj->next = heap;
    heap = obj;
    return obj;
}

void add_to_root_set(Object* obj) {
    obj->next = root_set.head;
    root_set.head = obj;
}

// Markierungsphase
void mark(Object* obj) {
    if (obj == NULL || obj->marked) return;
    obj->marked = true;
    for (int i = 0; i < obj->reference_count; i++) {
        mark(obj->references[i]);
    }
}

void mark_all() {
    Object* current = root_set.head;
    while (current != NULL) {
        mark(current);
        current = current->next;
    }
}

// Sweep-Phase
void sweep() {
    Object** current = &heap;
    while (*current != NULL) {
        if (!(*current)->marked) {
            // Objekt freigeben
            Object* unreferenced = *current;
            *current = unreferenced->next;
            if (unreferenced->references != NULL) {
                free(unreferenced->references);
            }
            free(unreferenced);
        } else {
            // Markierung für nächsten Zyklus entfernen
            (*current)->marked = false;
            current = &(*current)->next;
        }
    }
}

// Garbage Collection starten
void gc_collect() {
    mark_all();
    sweep();
}

// Beispiel-Programm
int main() {
    // Beispielhafte Erstellung von Objekten
    Object* obj1 = create_object(2);
    Object* obj2 = create_object(0);
    Object* obj3 = create_object(0);

    // Referenzen setzen
    obj1->references[0] = obj2;
    obj1->references[1] = obj3;

    // Zum Root-Set hinzufügen
    add_to_root_set(obj1);

    // Zyklus erstellen
    obj2->references = (Object**)malloc(sizeof(Object*));
    obj2->references[0] = obj1;
    obj2->reference_count = 1;

    // Garbage Collection starten
    gc_collect();

    return 0;
}
