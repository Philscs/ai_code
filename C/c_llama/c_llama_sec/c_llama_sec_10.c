#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

// Strukt für den Shared Memory-Bereich
typedef struct {
    int mutex;          // Mutex-Flag
    int flag;           // Flag zur Verwaltung des Shared Memory-Bereichs
    int data;           // Datenbereich des Shared Memory
} shared_memory_t;

shared_memory_t* shm = NULL;
mutex_t* shm_mutex = NULL;

// Funktion zum Erstellen des Shared Memory-Bereichs
shared_memory_t* shm_open(const char* name, int flags) {
    shm = (shared_memory_t*)malloc(sizeof(shared_memory_t));
    if (!shm) {
        return NULL;
    }

    // Öffnen des Shared Memory-Bereichs
    shm->flag = 0;
    shm->mutex = 1;

    // Verwenden von posix_memlock() zum Memprotktion
    shm->data = mmap(NULL, sizeof(shared_memory_t), PROT_READ | PROT_WRITE, MAP_SHARED, 
open("/dev/shm/" name, O_RDWR | O_CREAT), 0);
    if (shm->data == MAP_FAILED) {
        free(shm);
        return NULL;
    }

    // Initialisierung des Shared Memory-Bereichs
    shm->flag = 1;

    return shm;
}

// Funktion zum Schließen des Shared Memory-Bereichs
void shm_close(shared_memory_t* shm) {
    if (shm) {
        // Sperren der Nutzung des Shared Memory-Bereichs
        munmap(shm->data, sizeof(shared_memory_t));

        // Entfernen des Shared Memory-Bereichs
        unlink("/dev/shm/" shm->data);
    }
}

// Funktion zum Lesen aus dem Shared Memory-Bereich
int shm_read(int* buffer) {
    int bytes = read(shm->data, buffer, sizeof(int));
    return bytes;
}

// Funktion zum Schreiben in den Shared Memory-Bereich
void shm_write(const void* buffer) {
    write(shm->data, buffer, sizeof(int));
}

// Funktion zum Verwenden des Mutexs
int shm_mutex_lock() {
    int ret = pthread_mutex_lock(&shm_mutex->mutex);
    if (ret != 0) {
        printf("Fehler beim Locken des Mutexs\n");
    }
    return ret;
}

// Funktion zum Freigeben des Mutexs
void shm_mutex_unlock() {
    pthread_mutex_unlock(&shm_mutex->mutex);
}

int main() {
    // Erstellen eines Mutex-Variables
    shm_mutex = (mutex_t*)malloc(sizeof(mutex_t));
    pthread_mutex_init(&shm_mutex->mutex, NULL);

    // Erstellen des Shared Memory-Bereichs
    shm_open("shared_data", 0644);

    // Schreiben in den Shared Memory-Bereich
    int data = 10;
    shm_write((void*)&data);

    // Lesen aus dem Shared Memory-Bereich
    int buffer[1];
    shm_read(buffer);
    printf("Received value: %d\n", buffer[0]);

    // Freigeben des Mutexs
    shm_mutex_unlock();

    // Schließen des Shared Memory-Bereichs und des Prozesses
    shm_close(shm);
    pthread_join(0, NULL);
    pthread_mutex_destroy(&shm_mutex->mutex);
    free(shm_mutex);

    return 0;
}