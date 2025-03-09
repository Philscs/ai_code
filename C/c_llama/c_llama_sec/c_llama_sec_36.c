#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

// Variable zur Speicherung des aktiven Prozesses
int active_process = 0;

// Semaphor zum Synchronisieren der Kommunikation zwischen Prozessen
sem_t comm_sem;

// Mutex zum Schutz der Kommunikationsfunktionen
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Variable zur Speicherung der verfügbaren Ressourcen
int available_resources = 0;

void *send_message(void *arg) {
    int dest_process;
    char *message;

    // Überprüfung der Eingabewerte
    if (active_process == 0) {
        printf("No active process\n");
        return NULL;
    }

    dest_process = *(int *)arg;
    message = *(char **)arg;

    // Überprüfung der Nachricht
    if (strlen(message) == 0) {
        printf("Message cannot be empty\n");
        return NULL;
    }

    sem_wait(&comm_sem);
    pthread_mutex_lock(&mutex);
    active_process = dest_process;
    pthread_mutex_unlock(&mutex);

    // Aktivierung des Receiving-Prozesses
    receive_message(dest_process, message);

    sem_post(&comm_sem);
    pthread_mutex_lock(&mutex);
    active_process = 0;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *receive_message(void *arg) {
    int dest_process;
    char *message;

    // Überprüfung der Eingabewerte
    if (active_process == 0) {
        printf("No active process\n");
        return NULL;
    }

    dest_process = *(int *)arg;
    message = *(char **)arg;

    sem_wait(&comm_sem);
    pthread_mutex_lock(&mutex);
    active_process = dest_process;
    pthread_mutex_unlock(&mutex);

    // Speicherung der Nachricht in einem temporären Buffer
    strcpy(message, "Hello from message 3");

    sem_post(&comm_sem);
    pthread_mutex_lock(&mutex);
    active_process = 0;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void start_communication() {
    int dest_process;
    char *message;

    // Überprüfung der verfügbaren Ressourcen
    if (available_resources == 0) {
        printf("No resources available\n");
        return;
    }

    dest_process = 1;
    message = "Hello from message 1";

    sem_init(&comm_sem, 0, 1);
    pthread_mutex_init(&mutex, NULL);

    // Erstellung eines neuen Prozesses
    pthread_create(&active_process, NULL, send_message, &dest_process);

    printf("Message sent successfully\n");
}

int main() {
    start_communication();

    return 0;
}
