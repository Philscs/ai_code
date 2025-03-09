#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

// Maximale Größe der Queue
#define MAX_QUEUE_SIZE 1000
#define MAX_MSG_SIZE 1024

// Fehler-Codes
#define QUEUE_SUCCESS 0
#define QUEUE_ERROR_FULL -1
#define QUEUE_ERROR_EMPTY -2
#define QUEUE_ERROR_INVALID -3
#define QUEUE_ERROR_MEMORY -4
#define QUEUE_ERROR_INIT -5

// Prioritätsstufen
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_MEDIUM = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_LEVELS = 3
} MessagePriority;

// Message Struktur
typedef struct {
    uint8_t data[MAX_MSG_SIZE];
    size_t size;
    MessagePriority priority;
    uint64_t timestamp;
} Message;

// Queue Struktur
typedef struct {
    Message* messages[PRIORITY_LEVELS][MAX_QUEUE_SIZE];
    size_t front[PRIORITY_LEVELS];
    size_t rear[PRIORITY_LEVELS];
    size_t count[PRIORITY_LEVELS];
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    bool is_initialized;
    size_t total_memory;
    size_t max_memory;
} MessageQueue;

// Funktionsprototypen
int queue_init(MessageQueue* queue, size_t max_memory);
void queue_destroy(MessageQueue* queue);
int queue_push(MessageQueue* queue, const uint8_t* data, size_t size, MessagePriority priority);
int queue_pop(MessageQueue* queue, Message* msg);
bool queue_is_empty(MessageQueue* queue);
bool queue_is_full(MessageQueue* queue, size_t msg_size);

// Überprüfe ob Queue leer ist
bool queue_is_empty(MessageQueue* queue) {
    if (!queue || !queue->is_initialized) {
        return true;
    }

    for (int i = 0; i < PRIORITY_LEVELS; i++) {
        if (queue->count[i] > 0) {
            return false;
        }
    }
    return true;
}

// Überprüfe ob Queue voll ist
bool queue_is_full(MessageQueue* queue, size_t msg_size) {
    if (!queue || !queue->is_initialized) {
        return true;
    }

    return (queue->total_memory + msg_size > queue->max_memory);
}

// Initialisierung der Queue
int queue_init(MessageQueue* queue, size_t max_memory) {
    if (!queue || max_memory == 0) {
        return QUEUE_ERROR_INVALID;
    }

    // Initialisiere Mutex und Condition Variables
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        return QUEUE_ERROR_INIT;
    }
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        return QUEUE_ERROR_INIT;
    }
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        pthread_cond_destroy(&queue->not_empty);
        return QUEUE_ERROR_INIT;
    }

    // Initialisiere Queue-Statusvariablen
    for (int i = 0; i < PRIORITY_LEVELS; i++) {
        queue->front[i] = 0;
        queue->rear[i] = 0;
        queue->count[i] = 0;
        memset(queue->messages[i], 0, sizeof(Message*) * MAX_QUEUE_SIZE);
    }

    queue->total_memory = 0;
    queue->max_memory = max_memory;
    queue->is_initialized = true;

    return QUEUE_SUCCESS;
}

// Aufräumen der Queue
void queue_destroy(MessageQueue* queue) {
    if (!queue || !queue->is_initialized) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);

    // Freigabe aller noch vorhandenen Nachrichten
    for (int i = 0; i < PRIORITY_LEVELS; i++) {
        for (size_t j = 0; j < MAX_QUEUE_SIZE; j++) {
            free(queue->messages[i][j]);
            queue->messages[i][j] = NULL;
        }
    }

    pthread_mutex_unlock(&queue->mutex);

    // Zerstöre Synchronisationsobjekte
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);

    queue->is_initialized = false;
}

// Nachricht in die Queue einfügen
int queue_push(MessageQueue* queue, const uint8_t* data, size_t size, MessagePriority priority) {
    if (!queue || !queue->is_initialized || !data || size == 0 || 
        size > MAX_MSG_SIZE || priority >= PRIORITY_LEVELS) {
        return QUEUE_ERROR_INVALID;
    }

    pthread_mutex_lock(&queue->mutex);

    // Überprüfe Speicherlimit
    while (queue->total_memory + size > queue->max_memory) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    // Überprüfe ob Queue voll ist
    if (queue->count[priority] >= MAX_QUEUE_SIZE) {
        pthread_mutex_unlock(&queue->mutex);
        return QUEUE_ERROR_FULL;
    }

    // Allokiere neue Nachricht
    Message* msg = (Message*)malloc(sizeof(Message));
    if (!msg) {
        pthread_mutex_unlock(&queue->mutex);
        return QUEUE_ERROR_MEMORY;
    }

    // Kopiere Daten
    memcpy(msg->data, data, size);
    msg->size = size;
    msg->priority = priority;
    msg->timestamp = (uint64_t)time(NULL);

    // Füge Nachricht in Queue ein
    queue->messages[priority][queue->rear[priority]] = msg;
    queue->rear[priority] = (queue->rear[priority] + 1) % MAX_QUEUE_SIZE;
    queue->count[priority]++;
    queue->total_memory += size;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);

    return QUEUE_SUCCESS;
}

// Nachricht aus der Queue entnehmen
int queue_pop(MessageQueue* queue, Message* msg) {
    if (!queue || !queue->is_initialized || !msg) {
        return QUEUE_ERROR_INVALID;
    }

    pthread_mutex_lock(&queue->mutex);

    // Warte wenn Queue leer ist
    while (queue_is_empty(queue)) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    // Finde höchste nicht-leere Priorität
    int priority = PRIORITY_LEVELS - 1;
    while (priority >= 0 && queue->count[priority] == 0) {
        priority--;
    }

    if (priority < 0) {
        pthread_mutex_unlock(&queue->mutex);
        return QUEUE_ERROR_EMPTY;
    }

    // Hole Nachricht aus Queue
    Message* queued_msg = queue->messages[priority][queue->front[priority]];
    memcpy(msg, queued_msg, sizeof(Message));

    // Aktualisiere Queue-Status
    queue->messages[priority][queue->front[priority]] = NULL;
    queue->front[priority] = (queue->front[priority] + 1) % MAX_QUEUE_SIZE;
    queue->count[priority]--;
    queue->total_memory -= msg->size;

    free(queued_msg);

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);

    return QUEUE_SUCCESS;
}

// Producer Thread Funktion
void* producer(void* arg) {
    MessageQueue* queue = (MessageQueue*)arg;
    const char* messages[] = {"Low priority message", 
                            "Medium priority message", 
                            "High priority message"};

    for (int i = 0; i < 10; i++) {
        for (int prio = 0; prio < PRIORITY_LEVELS; prio++) {
            const char* msg = messages[prio];
            if (queue_push(queue, (uint8_t*)msg, strlen(msg) + 1, 
                          (MessagePriority)prio) == QUEUE_SUCCESS) {
                printf("Produced: %s\n", msg);
            }
        }
    }
    return NULL;
}

// Consumer Thread Funktion
void* consumer(void* arg) {
    MessageQueue* queue = (MessageQueue*)arg;
    Message msg;

    for (int i = 0; i < 30; i++) {
        if (queue_pop(queue, &msg) == QUEUE_SUCCESS) {
            printf("Consumed [Priority %d]: %s\n", 
                   msg.priority, (char*)msg.data);
        }
    }
    return NULL;
}

// Hauptprogramm
int main() {
    MessageQueue queue;
    pthread_t prod_thread, cons_thread;

    // Initialisiere Queue mit 1MB Speicherlimit
    if (queue_init(&queue, 1024 * 1024) != QUEUE_SUCCESS) {
        printf("Failed to initialize queue\n");
        return 1;
    }

    // Starte Producer und Consumer Threads
    pthread_create(&prod_thread, NULL, producer, &queue);
    pthread_create(&cons_thread, NULL, consumer, &queue);

    // Warte auf Thread-Beendigung
    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    // Aufräumen
    queue_destroy(&queue);

    return 0;
}