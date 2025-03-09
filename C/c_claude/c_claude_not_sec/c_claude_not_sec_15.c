#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Maximale Anzahl von Referenzen pro Objekt
#define MAX_REFS 10

// Objektstruktur
typedef struct Object {
    int id;                         // Eindeutige ID des Objekts
    bool marked;                    // Markierungsflag für GC
    int ref_count;                  // Anzahl der Referenzen
    struct Object* refs[MAX_REFS];  // Array von Referenzen auf andere Objekte
    void* data;                     // Nutzdaten des Objekts
} Object;

// Heap-Struktur zur Verwaltung aller Objekte
typedef struct {
    Object** objects;    // Array aller Objekte
    int capacity;        // Maximale Anzahl von Objekten
    int count;          // Aktuelle Anzahl von Objekten
    Object* root;       // Wurzelobjekt (Startpunkt für Mark-and-Sweep)
} Heap;

// Heap initialisieren
Heap* create_heap(int capacity) {
    Heap* heap = (Heap*)malloc(sizeof(Heap));
    heap->capacity = capacity;
    heap->count = 0;
    heap->objects = (Object**)malloc(sizeof(Object*) * capacity);
    heap->root = NULL;
    return heap;
}

// Neues Objekt erstellen
Object* create_object(Heap* heap, int id, void* data) {
    if (heap->count >= heap->capacity) {
        return NULL;
    }

    Object* obj = (Object*)malloc(sizeof(Object));
    obj->id = id;
    obj->marked = false;
    obj->ref_count = 0;
    obj->data = data;

    for (int i = 0; i < MAX_REFS; i++) {
        obj->refs[i] = NULL;
    }

    heap->objects[heap->count++] = obj;
    return obj;
}

// Referenz zwischen Objekten erstellen
void add_reference(Object* from, Object* to) {
    if (from->ref_count < MAX_REFS) {
        from->refs[from->ref_count++] = to;
    }
}

// Markierungsphase (rekursiv)
void mark(Object* obj) {
    if (obj == NULL || obj->marked) {
        return;
    }

    // Objekt markieren
    obj->marked = true;

    // Rekursiv alle referenzierten Objekte markieren
    for (int i = 0; i < obj->ref_count; i++) {
        mark(obj->refs[i]);
    }
}

// Sweep-Phase
void sweep(Heap* heap) {
    for (int i = 0; i < heap->count; i++) {
        Object* obj = heap->objects[i];
        if (!obj->marked) {
            // Nicht markiertes Objekt löschen
            printf("Lösche Objekt %d\n", obj->id);
            free(obj->data);
            free(obj);
            
            // Objekt aus dem Heap entfernen
            for (int j = i; j < heap->count - 1; j++) {
                heap->objects[j] = heap->objects[j + 1];
            }
            heap->count--;
            i--; // Index anpassen, da wir ein Element entfernt haben
        } else {
            // Markierung für nächsten GC-Durchlauf zurücksetzen
            obj->marked = false;
        }
    }
}

// Hauptfunktion für Garbage Collection
void garbage_collect(Heap* heap) {
    printf("Starte Garbage Collection...\n");
    
    // Mark-Phase
    if (heap->root != NULL) {
        mark(heap->root);
    }
    
    // Sweep-Phase
    sweep(heap);
    
    printf("Garbage Collection abgeschlossen.\n");
}

// Heap freigeben
void destroy_heap(Heap* heap) {
    for (int i = 0; i < heap->count; i++) {
        Object* obj = heap->objects[i];
        free(obj->data);
        free(obj);
    }
    free(heap->objects);
    free(heap);
}

// Beispiel für die Verwendung
int main() {
    // Heap mit Kapazität für 10 Objekte erstellen
    Heap* heap = create_heap(10);
    
    // Beispielobjekte erstellen
    int* data1 = malloc(sizeof(int)); *data1 = 1;
    int* data2 = malloc(sizeof(int)); *data2 = 2;
    int* data3 = malloc(sizeof(int)); *data3 = 3;
    int* data4 = malloc(sizeof(int)); *data4 = 4;
    
    Object* obj1 = create_object(heap, 1, data1);
    Object* obj2 = create_object(heap, 2, data2);
    Object* obj3 = create_object(heap, 3, data3);
    Object* obj4 = create_object(heap, 4, data4);
    
    // Wurzelobjekt setzen
    heap->root = obj1;
    
    // Referenzen erstellen (inkl. Zyklus)
    add_reference(obj1, obj2);
    add_reference(obj2, obj3);
    add_reference(obj3, obj4);
    add_reference(obj4, obj2); // Zyklus: 2 -> 3 -> 4 -> 2
    
    // Garbage Collection durchführen
    garbage_collect(heap);
    
    // Heap aufräumen
    destroy_heap(heap);
    
    return 0;
}