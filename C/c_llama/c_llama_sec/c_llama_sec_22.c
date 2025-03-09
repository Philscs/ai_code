#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 10
#define ITEMS_PER_BATCH 5

// Struktur für den Ringbuffer
typedef struct {
    int buffer[BUFFER_SIZE];
    int head;
    int tail;
    int size;
} ring_buffer;

ring_buffer rb;

// Initialisierung des Ringbuffers
void init_ringbuffer() {
    rb.head = 0;
    rb.tail = 0;
    rb.size = 0;
}

// Hinzufügen eines Items zum Ringbuffer
int add_item(int item) {
    int result;
    
    // Atomische Operation zur Überprüfung der Verfügbarkeit des Buffers
    if (rb.size < ITEMS_PER_BATCH) {
        int old_tail = rb.tail;
        
        // Änderung der Tail-Adresse mit einem Memory-Break
        __sync_lock_test_and_set(&rb.tail, rb.head);
        
        // Hinzufügen des Items zum Buffer
        result = rb.buffer[old_tail];
        rb.buffer[old_tail] = item;
        
        // Atomische Operation zur Überprüfung der neuen Größe des Buffers
        if (__sync_bool_compare_and_swap(&rb.size, 0, 1)) {
            // Memory-Break für die Änderung der Head-Adresse
            __sync_lock_test_and_set(&rb.head, rb.tail + 1);
            
            return result;
        }
    } else {
        printf("Ringbuffer vollständig\n");
        exit(1);
    }
    
    return -1; // Fehler: Ringbuffer vollständig
}

// Entleeren eines Items aus dem Ringbuffer
int remove_item() {
    int item;
    
    if (rb.size > 0) {
        int old_tail = rb.tail;
        
        // Änderung der Tail-Adresse mit einem Memory-Break
        __sync_lock_test_and_set(&rb.tail, rb.head);
        
        // Entleeren des Items aus dem Buffer
        item = rb.buffer[old_tail];
        
        // Atomische Operation zur Überprüfung der neuen Größe des Buffers
        if (__sync_bool_compare_and_swap(&rb.size, 1, 0)) {
            // Memory-Break für die Änderung der Head-Adresse
            __sync_lock_test_and_set(&rb.head, rb.tail + 1);
            
            return item;
        }
    } else {
        printf("Ringbuffer leer\n");
        exit(1);
    }
    
    return -1; // Fehler: Ringbuffer leer
}

// Funktion für das Erzeugen eines neuen Threads
void* worker(void* arg) {
    for (int i = 0; i < 10; i++) {
        add_item(i);
    }
    return NULL;
}

int main() {
    init_ringbuffer();
    
    pthread_t threads[5];
    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }
    
    for (int i = 0; i < 10; i++) {
        remove_item();
    }
    
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}