#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// Definition einer Arbeit (Task)
typedef struct {
    void (*func)(void *);
    void *arg;
} task_t;

// Sichere Queue Implementierung mit Mutex und CondVar
#define MAX_TASKS 10

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int size;
    int capacity;
    task_t tasks[MAX_TASKS];
} safe_queue_t;

void init_safe_queue(safe_queue_t *q) {
    q->size = 0;
    q->capacity = MAX_TASKS;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void add_task_to_queue(safe_queue_t *q, task_t task) {
    int head_index = (q->size + 1) % MAX_TASKS;

    // Einfügen der Aufgabe in die Prioritätenliste
    for (int i = q->size; i >= head_index; --i) {
        if (!q->tasks[i].func) {  // Wenn die Aufgabe bereits verarbeitet wurde, springen wir zum 
Anfang
            break;
        }
    }

    pthread_mutex_lock(&q->mutex);
    q->tasks[head_index] = task;
    q->size = (q->size + 1) % MAX_TASKS;

    if (q->size == MAX_TASKS) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    pthread_mutex_unlock(&q->mutex);
}

void process_task(safe_queue_t *q) {
    while (q->size > 0) {
        task_t current = q->tasks[(q->size - 1 + MAX_TASKS - 1) % MAX_TASKS];

        if (current.func == NULL) { // Aufgabe bereits verarbeitet
            pthread_cond_wait(&q->cond, &q->mutex);
            continue;
        }

        // Verarbeiten der Aufgabe
        current.func(current.arg);

        pthread_mutex_lock(&q->mutex);
        q->size--;
        pthread_mutex_unlock(&q->mutex);

        if (q->size == 0) {
            pthread_cond_signal(&q->cond);
        }
    }
}

// Thread-Sanitization: Verwenden von Mutex für Zugriffsschutz
#define NUM_THREADS 4

typedef struct {
    safe_queue_t *queue;
    pthread_mutex_t mutex;
} thread_pool_t;

void init_thread_pool(thread_pool_t *pool, void (*task)(void *)) {
    pool->queue = (safe_queue_t *)malloc(sizeof(safe_queue_t));
    init_safe_queue(pool->queue);
    pool->mutex = pthread_mutex_init(&pool->mutex, NULL);

    // Erstelle ein Thread-Array
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, process_task, pool->queue) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    // Setze die Priorität der Aufgaben
    for (int i = 0; i < NUM_THREADS; ++i) {
        task(&threads[i]);
    }
}

void thread_main(void *arg) {
    void (*func)(void *) = (task_t*)arg;

    while (1) {
        if (!func(NULL)) { // Aufgabe bereits verarbeitet
            break;
        }

        func(NULL);  // Verarbeiten der Aufgabe
    }
}

// Resource Limiting: Setze die Anzahl der verfügbaren Threads auf MAX_THREADS
#define MAX_THREADS 4

void set_thread_limit(thread_pool_t *pool, int limit) {
    pthread_mutex_lock(&pool->mutex);
    pool->queue->capacity = limit;
    pthread_mutex_unlock(&pool->mutex);

    // Warten, bis alle verfügbaren Threads freigesetzt werden
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }
}

// Deadlock Prevention: Verwenden von Semaphoren für Konditionierung
typedef struct {
    sem_t *semaphore;
} deadlock_detector_t;

void init_deadlock_detector(deadlock_detector_t *detector) {
    detector->semaphore = (sem_t *)malloc(sizeof(sem_t));
    sem_init(detector->semaphore, 0, NUM_THREADS);
}

void deadlock_detector_main(deadlock_detector_t *detector) {
    int num_threads = 0;
    void (*func)(void *) = NULL;

    // Warten auf ein neues Thread
    sem_wait(detector->semaphore);

    while (1) {
        if (!func) { // Keine Aufgabe verarbeitet
            break;
        }

        func(NULL);  // Verarbeiten der Aufgabe
    }
}

// Task Prioritization: Verwenden von Mutex für Aufgabenpriorisierung
typedef struct {
    pthread_mutex_t mutex;
} task_prioritizer_t;

void init_task_prioritizer(task_prioritizer_t *prioritizer) {
    pthread_mutex_init(&prioritizer->mutex, NULL);
}

int compare_tasks(void *a, void *b) {
    task_t *t1 = (task_t *)a;
    task_t *t2 = (task_t *)b;

    return t1->func - t2->func; // Aufgaben sortieren nach Priorität
}
