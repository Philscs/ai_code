#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <time.h>

#define MAX_QUEUE_SIZE 100
#define MAX_THREADS 10
#define DEADLOCK_TIMEOUT 5 // Sekunden

typedef struct {
    int data;
    int priority;
} QueueItem;

typedef struct {
    QueueItem items[MAX_QUEUE_SIZE];
    int front;
    int rear;
    atomic_int size;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    pthread_t owner;           // Aktueller Lock-Besitzer
    time_t lock_time;         // Zeitpunkt der Lock-Übernahme
    bool is_locked;
} PriorityQueue;

// Globale Datenstruktur für Deadlock-Detection
typedef struct {
    pthread_t thread_id;
    PriorityQueue* waiting_for;
    PriorityQueue* holding;
} ThreadStatus;

ThreadStatus thread_status[MAX_THREADS];
pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialisierung der Queue
void queue_init(PriorityQueue* queue) {
    queue->front = 0;
    queue->rear = -1;
    atomic_init(&queue->size, 0);
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    queue->is_locked = false;
}

// Überprüfung auf Deadlock
bool check_deadlock(PriorityQueue* queue) {
    pthread_mutex_lock(&status_mutex);
    
    time_t current_time = time(NULL);
    if (queue->is_locked && (current_time - queue->lock_time) > DEADLOCK_TIMEOUT) {
        pthread_mutex_unlock(&status_mutex);
        return true;
    }
    
    pthread_mutex_unlock(&status_mutex);
    return false;
}

// Recovery Mechanismus
void recover_from_deadlock(PriorityQueue* queue) {
    pthread_mutex_lock(&status_mutex);
    
    // Force-Release des Locks
    if (queue->is_locked) {
        queue->is_locked = false;
        queue->owner = 0;
        pthread_cond_broadcast(&queue->not_full);
        pthread_cond_broadcast(&queue->not_empty);
    }
    
    pthread_mutex_unlock(&status_mutex);
}

// Thread-sicheres Einfügen mit Priorität
bool queue_push(PriorityQueue* queue, int data, int priority) {
    pthread_mutex_lock(&queue->mutex);
    
    // Deadlock Detection
    queue->lock_time = time(NULL);
    queue->is_locked = true;
    queue->owner = pthread_self();
    
    while (atomic_load(&queue->size) >= MAX_QUEUE_SIZE) {
        if (check_deadlock(queue)) {
            recover_from_deadlock(queue);
            pthread_mutex_unlock(&queue->mutex);
            return false;
        }
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    // Finde die richtige Position basierend auf Priorität
    int pos = queue->rear;
    while (pos >= 0 && queue->items[pos].priority < priority) {
        queue->items[pos + 1] = queue->items[pos];
        pos--;
    }
    
    // Füge neues Element ein
    queue->items[pos + 1].data = data;
    queue->items[pos + 1].priority = priority;
    queue->rear++;
    atomic_fetch_add(&queue->size, 1);
    
    queue->is_locked = false;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

// Thread-sicheres Entfernen
bool queue_pop(PriorityQueue* queue, int* data) {
    pthread_mutex_lock(&queue->mutex);
    
    // Deadlock Detection
    queue->lock_time = time(NULL);
    queue->is_locked = true;
    queue->owner = pthread_self();
    
    while (atomic_load(&queue->size) == 0) {
        if (check_deadlock(queue)) {
            recover_from_deadlock(queue);
            pthread_mutex_unlock(&queue->mutex);
            return false;
        }
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    *data = queue->items[queue->front].data;
    queue->front++;
    atomic_fetch_sub(&queue->size, 1);
    
    if (queue->front > queue->rear) {
        queue->front = 0;
        queue->rear = -1;
    }
    
    queue->is_locked = false;
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

// Aufräumen der Queue-Ressourcen
void queue_destroy(PriorityQueue* queue) {
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
}

// Beispiel für die Verwendung
void* producer(void* arg) {
    PriorityQueue* queue = (PriorityQueue*)arg;
    for (int i = 0; i < 5; i++) {
        int priority = rand() % 10;
        if (queue_push(queue, i, priority)) {
            printf("Produziert: %d mit Priorität %d\n", i, priority);
        }
        usleep(100000);  // 100ms Pause
    }
    return NULL;
}

void* consumer(void* arg) {
    PriorityQueue* queue = (PriorityQueue*)arg;
    int data;
    for (int i = 0; i < 5; i++) {
        if (queue_pop(queue, &data)) {
            printf("Konsumiert: %d\n", data);
        }
        usleep(150000);  // 150ms Pause
    }
    return NULL;
}

int main() {
    PriorityQueue queue;
    queue_init(&queue);
    
    pthread_t prod_thread, cons_thread;
    pthread_create(&prod_thread, NULL, producer, &queue);
    pthread_create(&cons_thread, NULL, consumer, &queue);
    
    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);
    
    queue_destroy(&queue);
    return 0;
}