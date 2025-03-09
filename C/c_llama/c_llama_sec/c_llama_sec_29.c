#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_OBJECTS 1000

typedef struct {
    void* memory;
    size_t size;
} object_t;

object_t object_pool[MAX_OBJECTS];

int object_count = 0;

// Funktion, um ein neues Objekt aus dem Pool zu erhalten
void* get_object(void* arg) {
    object_t* obj = &object_pool[object_count];
    memcpy(obj->memory, malloc(obj->size), obj->size);
    object_count++;

    pthread_mutex_lock(&lock);
    printf("Objekt %d erstellt\n", object_count - 1);

    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);

    return (void*)obj;
}

// Funktion, um ein Objekt aus dem Pool zurückzugeben
void put_object(void* obj) {
    object_t* o = (object_t*)obj;

    pthread_mutex_lock(&lock);
    printf("Objekt %d zurückgegeben\n", object_count - 1);

    memcpy(o->memory, NULL, o->size);

    object_count--;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);
}

int main() {
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    // Erstellen Sie ein neues Objekt aus dem Pool
    void* obj = get_object(NULL);

    // Verwenden Sie das Objekt
    printf("Objekt: ");
    for (int i = 0; i < strlen((char*)obj); ++i) {
        printf("%c", ((char*)obj)[i]);
    }
    printf("\n");

    // Das Objekt aus dem Pool zurückgeben
    put_object(obj);

    return 0;
}