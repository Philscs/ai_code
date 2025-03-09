#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>

// Definition der Shared Memory-Region
#define SHARED_MEMORY_REGION 1024

// Definition des Semaphors
#define SEMAPHOR 1

// Definition des Message Queues
typedef struct {
    int data;
} message_t;

int main() {
    // Erstellen Sie eine neue Semaphore
    sem_t *semaphore = sem_open("/semaphore", O_CREAT, 0644, SEMAPHOR);

    // Binden Sie den Semaphor an die Shared Memory-Region
    sem_unlink("/semaphore");

    // Erstellen Sie eine neue Message Queue
    message_t *message_queue = malloc(sizeof(message_t));

    // Binden Sie die Message Queue an die Shared Memory-Region
    sem_unlink("/semaphore");

    // Öffnen Sie den Zugriff auf die Shared Memory-Region
    int shared_memory_fd = open("/shared_memory", O_RDWR | O_CREAT, 0644);
    void *shared_memory_addr = mmap(NULL, SHARED_MEMORY_REGION, PROT_READ | PROT_WRITE, 
MAP_SHARED, shared_memory_fd, 0);

    // Erstellen Sie eine neue Pthread-Gruppe
    pthread_t thread;

    // Erstellen Sie einen neuen Thread
    if (pthread_create(&thread, NULL, send_message, message_queue) != 0) {
        printf("Fehler beim Erstellen des Threads\n");
        return 1;
    }

    // Warten auf den Abschluss der Message-Sendung
    pthread_join(thread, NULL);

    // Schließen Sie alle Offenberechtigungen
    close(shared_memory_fd);
    munmap(shared_memory_addr, SHARED_MEMORY_REGION);
    sem_close(semaphore);
    free(message_queue);

    return 0;
}

// Funktion zum Senden einer Nachricht über die Message Queue
void *send_message(void *arg) {
    message_t *message = (message_t *) arg;

    // Setze den Datenwert der Nachricht
    message->data = 10;

    // Senden Sie die Nachricht an den Empfänger
    sem_wait(semaphore);

    return NULL;
}

// Funktion zum Empfangen einer Nachricht über die Message Queue
void *receive_message(void *arg) {
    message_t *message = (message_t *) arg;

    // Lösen Sie den Semaphore, um die Nachricht zu empfangen
    sem_post(semaphore);

    // Setze den Datenwert der Nachricht
    message->data = 20;

    return NULL;
}