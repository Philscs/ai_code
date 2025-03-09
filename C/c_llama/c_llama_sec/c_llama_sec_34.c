#ifndef SHARED_MEMORY_MANAGER_H
#define SHARED_MEMORY_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Strukturfür die Shared Memory
typedef struct {
    void* data;
    int size;
    int permissions; // 0: Lesen nur, 1: Schreiben und lesen, 2: Exklusives Schreiben
} shared_memory_t;

// Funktion zum Erstellen einer neuen Shared Memory
shared_memory_t* create_shared_memory(int size);

// Funktion zum Erteilen von Permissions für eine Shared Memory
int grant_permissions(shared_memory_t* shm, int permissions);

// Funktion zur Übertragung von Daten in die Shared Memory
void write_to_shared_memory(shared_memory_t* shm, void* data, int size);

// Funktion zur Lesung von Daten aus der Shared Memory
void read_from_shared_memory(shared_memory_t* shm, void* data, int size);

// Funktion zum Beenden eines Thread-Loops
void thread_end(void* arg);

#endif // SHARED_MEMORY_MANAGER_H#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Funktion zum Erstellen einer neuen Shared Memory
shared_memory_t* create_shared_memory(int size) {
    shared_memory_t* shm = malloc(sizeof(shared_memory_t));
    shm->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
    shm->size = size;
    shm->permissions = 0; // Lesen nur
    return shm;
}

// Funktion zum Erteilen von Permissions für eine Shared Memory
int grant_permissions(shared_memory_t* shm, int permissions) {
    if (permissions == 1) { // Schreiben und lesen
        shm->permissions |= 2; // Exklusives Schreiben
    } else if (permissions == 2) { // Exklusives Schreiben
        shm->permissions |= 4; // Lesen nur
    }
    return 0;
}

// Funktion zur Übertragung von Daten in die Shared Memory
void write_to_shared_memory(shared_memory_t* shm, void* data, int size) {
    void* ptr = mmap(data, size, PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
    memcpy(ptr, data, size);
}

// Funktion zur Lesung von Daten aus der Shared Memory
void read_from_shared_memory(shared_memory_t* shm, void* data, int size) {
    void* ptr = mmap(data, size, PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
    memcpy(data, ptr, size);
}

// Funktion zum Beenden eines Thread-Loops
void thread_end(void* arg) {
    free(arg);
}

int main() {
    shared_memory_t* shm = create_shared_memory(1024);
    if (shm == NULL) {
        printf("Fehler beim Erstellen der Shared Memory\n");
        return 1;
    }

    // Grant Permissions für die Shared Memory
    int permissions = grant_permissions(shm, 2); // Exklusives Schreiben

    if (permissions != 0) {
        printf("Fehler beim Vergeben von Permissions\n");
        return 1;
    }

    // Übertragen von Daten in die Shared Memory
    void* data = malloc(1024);
    for (int i = 0; i < 1024; i++) {
        data[i] = 'A' + i;
    }
    write_to_shared_memory(shm, data, 1024);

    // Lesen von Daten aus der Shared Memory
    void* read_data = malloc(1024);
    read_from_shared_memory(shm, read_data, 1024);

    printf("Lesene Daten:\n");
    for (int i = 0; i < 1024; i++) {
        printf("%c", read_data[i]);
    }
    free(read_data);

    // Beenden des Threads
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, thread_end, shm);
    pthread_join(thread_id, NULL);

    return 0;
}