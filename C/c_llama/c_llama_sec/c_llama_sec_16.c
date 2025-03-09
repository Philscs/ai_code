#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_SIZE 1024
#define HASH_FUNCTION(x) ((x) * 31 + (x) << 5)

typedef struct {
    int* data;
    int size;
    int capacity;
} HashTable;

// Initialisiere einen neuen Hashtablen mit der angegebenen Größe
HashTable* hash_table_init() {
    HashTable* table = malloc(sizeof(HashTable));
    table->data = malloc(MAX_SIZE * sizeof(int));
    table->size = 0;
    table->capacity = MAX_SIZE;

    return table;
}

// Berechnet die Index des Elements mit der angegebenen Schlüssel im Hashtablen
int hash_function(const void* key, size_t size) {
    const char* str = (const char*)key;
    int x = 0;
    for (size_t i = 0; i < size; ++i)
        x = HASH_FUNCTION(x) + (str[i] - 'a' + 1);
    return x % MAX_SIZE;
}

// Iteriert über alle Schlüssel in einem Hashtablen
void hash_table_iterate(HashTable* table, void (*callback)(const char*, int)) {
    for (int i = 0; i < table->capacity; ++i) {
        if (table->data[i] != NULL) {
            callback((char*)table->data[i], i);
        }
    }
}

// Einführt eine Schlüssel-Wert-Paar in den Hashtablen
void hash_table_insert(HashTable* table, const char* key, int value) {
    int index = hash_function(key, strlen(key));
    if (table->size >= table->capacity - 10) { // Reserviere Platz für neue Einträge
        table->capacity *= 2;
        table->data = realloc(table->data, table->capacity * sizeof(int));

        for (int i = 0; i < table->capacity; ++i)
            if (table->data[i] == NULL) {
                break;
            }
    }

    if (table->data[index] != NULL) { // Wenn der Index bereits gesetzt ist
        int j = index + 1;
        while (j < table->capacity && table->data[j] != NULL)
            ++j;

        if (j < table->capacity) {
            // Fall durch: Schlüssel doppelt vorhanden
            printf("Schlüssel %s bereits vorhanden\n", key);
            return;
        }

        for (; j >= index; --j)
            if (table->data[j] == NULL) {
                break;
            }
    }

    table->data[index] = value;
    ++table->size;

    printf("Schlüssel %s und Wert %d eingefügt\n", key, value);
}

// Lese den Wert für einen bestimmten Schlüssel aus dem Hashtablen
int hash_table_get(HashTable* table, const char* key) {
    int index = hash_function(key, strlen(key));
    if (table->data[index] != NULL)
        return *table->data + index;

    printf("Schlüssel %s nicht gefunden\n", key);
    return -1;
}

// Lösche eine Schlüssel-Wert-Paar aus dem Hashtablen
void hash_table_remove(HashTable* table, const char* key) {
    int index = hash_function(key, strlen(key));
    if (table->data[index] == NULL)
        printf("Schlüssel %s nicht gefunden\n", key);
    else {
        printf("Schlüssel %s entfernt\n", key);

        // Löse den Wert aus der Datenbank
        for (int i = index; table->data[i] != NULL; ++i) {
            if (table->data[i] == *table->data + i)
                break;
        }

        // Verwende einen anderen Index für die Freigabe des Speicherplatzes
        int j = index + 1;
        while (j < table->capacity && table->data[j] != NULL)
            ++j;

        if (j < table->capacity) {
            for (; j > i; --j)
                *table->data++ = *table->data--;
        }

        // Löse die Datenbank
        free(table->data);
    }
}

// Führe einen bestimmten Wert aus dem Hashtablen auf das Speicherplatz zurück
void* hash_table_get_value(HashTable* table, const char* key) {
    int index = hash_function(key, strlen(key));
    if (table->data[index] == NULL)
        return NULL;

    void* value = malloc(sizeof(int));
    *value = *(int*)table->data + index;
    free(table->data);
    return value;
}

// Ersetze den Wert für einen bestimmten Schlüssel im Hashtablen
void hash_table_set(HashTable* table, const char* key, int value) {
    int index = hash_function(key, strlen(key));
    if (table->data[index] == NULL)
        *table->data += index;

    for (int i = index; table->data[i] != NULL; ++i)
        if (table->data[i] == *(int*)table->data + i)
            break;
}

// Freigebe den Hashtablen und seinen Speicherplatz
void hash_table_free(HashTable* table) {
    free(table->data);
    free(table);
}

// Verwende diese Funktion, um Schlüssel-Wert-Paare zu übergeben, die in einem bestimmten 
Hashtablen gespeichert werden sollen.
int main() {
    HashTable* table = hash_table_init();

    // Einführen mehrere Schlüssel-Wert-Paare
    hash_table_insert(table, "Schlüssel1", 10);
    hash_table_insert(table, "Schlüssel2", 20);

    // Lese den Wert für einen bestimmten Schlüssel aus dem Hashtablen
    printf("%d\n", *(int*)hash_table_get_value(table, "Schlüssel1"));

    // Überprüfe, ob ein bestimmter Schlüssel vorhanden ist
    if (hash_table_get(table, "Schlüssel3") == -1)
        printf("Schlüssel 3 nicht gefunden\n");

    // Setze einen neuen Wert für einen bestimmten Schlüssel im Hashtablen
    hash_table_set(table, "Schlüssel2", 30);

    // Lese den neuen Wert aus dem Hashtablen
    printf("%d\n", *(int*)hash_table_get_value(table, "Schlüssel2"));

    // Lösche einen bestimmten Schlüssel aus dem Hashtablen
    hash_table_remove(table, "Schlüssel1");

    return 0;
}