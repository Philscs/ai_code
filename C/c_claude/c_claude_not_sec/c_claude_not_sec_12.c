/**
 * Thread Pool Implementation mit Work Stealing und dynamischer Skalierung
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <unistd.h>

// Konfigurationsparameter
#define LOAD_CHECK_INTERVAL 1000  // ms
#define TARGET_LOAD 0.7
#define SCALING_THRESHOLD 0.2

// Task-Struktur für die Work Queue
typedef struct Task {
    void (*function)(void*);
    void* arg;
    struct Task* next;
} Task;

// Deque für Work Stealing
typedef struct Deque {
    Task* top;
    Task* bottom;
    atomic_int size;
    pthread_mutex_t mutex;
} Deque;

// Worker-Struktur
typedef struct Worker {
    pthread_t thread;
    Deque* local_queue;
    struct ThreadPool* pool;
    int id;
    bool is_active;
} Worker;

// Thread Pool Struktur
typedef struct ThreadPool {
    Worker** workers;
    int min_threads;
    int max_threads;
    atomic_int current_threads;
    atomic_bool shutdown;
    pthread_mutex_t pool_mutex;
    pthread_cond_t pool_cond;
    double target_load;
} ThreadPool;

// Deque Operationen
Deque* deque_create(void) {
    Deque* deque = malloc(sizeof(Deque));
    if (!deque) return NULL;

    deque->top = NULL;
    deque->bottom = NULL;
    atomic_init(&deque->size, 0);
    pthread_mutex_init(&deque->mutex, NULL);

    return deque;
}

void deque_push(Deque* deque, Task* task) {
    pthread_mutex_lock(&deque->mutex);

    if (!deque->top) {
        deque->top = deque->bottom = task;
    } else {
        task->next = deque->top;
        deque->top = task;
    }

    atomic_fetch_add(&deque->size, 1);
    pthread_mutex_unlock(&deque->mutex);
}

Task* deque_pop(Deque* deque) {
    pthread_mutex_lock(&deque->mutex);
    
    if (!deque->top) {
        pthread_mutex_unlock(&deque->mutex);
        return NULL;
    }

    Task* task = deque->top;
    deque->top = task->next;
    
    if (!deque->top) {
        deque->bottom = NULL;
    }

    atomic_fetch_sub(&deque->size, 1);
    pthread_mutex_unlock(&deque->mutex);

    task->next = NULL;
    return task;
}

Task* deque_steal(Deque* deque) {
    pthread_mutex_lock(&deque->mutex);
    
    if (!deque->bottom) {
        pthread_mutex_unlock(&deque->mutex);
        return NULL;
    }

    Task* task = deque->bottom;
    
    if (deque->top == deque->bottom) {
        deque->top = deque->bottom = NULL;
    } else {
        Task* current = deque->top;
        while (current->next != deque->bottom) {
            current = current->next;
        }
        current->next = NULL;
        deque->bottom = current;
    }

    atomic_fetch_sub(&deque->size, 1);
    pthread_mutex_unlock(&deque->mutex);

    task->next = NULL;
    return task;
}

void deque_destroy(Deque* deque) {
    if (!deque) return;

    pthread_mutex_lock(&deque->mutex);
    
    Task* current = deque->top;
    while (current) {
        Task* next = current->next;
        free(current);
        current = next;
    }

    pthread_mutex_unlock(&deque->mutex);
    pthread_mutex_destroy(&deque->mutex);
    free(deque);
}

// Forward Declarations für Thread Pool Funktionen
static void* worker_routine(void* arg);
static Task* try_steal_task(ThreadPool* pool, int worker_id);
static void adjust_pool_size(ThreadPool* pool);

// Hauptfunktionen des Thread Pools
ThreadPool* threadpool_create(int min_threads, int max_threads) {
    ThreadPool* pool = malloc(sizeof(ThreadPool));
    if (!pool) return NULL;

    pool->min_threads = min_threads;
    pool->max_threads = max_threads;
    pool->current_threads = min_threads;
    atomic_store(&pool->shutdown, false);
    pool->target_load = TARGET_LOAD;
    pthread_mutex_init(&pool->pool_mutex, NULL);
    pthread_cond_init(&pool->pool_cond, NULL);

    // Worker-Array initialisieren
    pool->workers = malloc(sizeof(Worker*) * max_threads);
    if (!pool->workers) {
        free(pool);
        return NULL;
    }

    // Initial workers erstellen
    for (int i = 0; i < min_threads; i++) {
        pool->workers[i] = malloc(sizeof(Worker));
        if (!pool->workers[i]) continue;

        pool->workers[i]->id = i;
        pool->workers[i]->pool = pool;
        pool->workers[i]->is_active = true;
        pool->workers[i]->local_queue = deque_create();
        pthread_create(&pool->workers[i]->thread, NULL, worker_routine, pool->workers[i]);
    }

    return pool;
}

static void* worker_routine(void* arg) {
    Worker* worker = (Worker*)arg;
    ThreadPool* pool = worker->pool;
    Deque* local_queue = worker->local_queue;

    while (!atomic_load(&pool->shutdown)) {
        // Versuche lokale Aufgabe zu bekommen
        Task* task = deque_pop(local_queue);
        
        if (!task) {
            // Work Stealing, wenn keine lokale Aufgabe verfügbar
            task = try_steal_task(pool, worker->id);
        }

        if (task) {
            task->function(task->arg);
            free(task);
            continue;
        }

        // Keine Arbeit verfügbar - prüfe Pool-Größe
        adjust_pool_size(pool);

        // Warte auf neue Aufgaben
        pthread_mutex_lock(&pool->pool_mutex);
        if (!atomic_load(&pool->shutdown)) {
            pthread_cond_wait(&pool->pool_cond, &pool->pool_mutex);
        }
        pthread_mutex_unlock(&pool->pool_mutex);
    }

    return NULL;
}

static Task* try_steal_task(ThreadPool* pool, int worker_id) {
    int current_size = atomic_load(&pool->current_threads);
    
    // Zufälligen Worker auswählen
    int victim_id = rand() % current_size;
    if (victim_id == worker_id) {
        victim_id = (victim_id + 1) % current_size;
    }

    Worker* victim = pool->workers[victim_id];
    return deque_steal(victim->local_queue);
}

static void adjust_pool_size(ThreadPool* pool) {
    pthread_mutex_lock(&pool->pool_mutex);
    
    int current = atomic_load(&pool->current_threads);
    int total_tasks = 0;
    int active_workers = 0;

    // Berechne Auslastung
    for (int i = 0; i < current; i++) {
        if (pool->workers[i]->is_active) {
            active_workers++;
            total_tasks += atomic_load(&pool->workers[i]->local_queue->size);
        }
    }

    double load = (double)total_tasks / active_workers;

    // Skalierung nach oben
    if (load > pool->target_load + SCALING_THRESHOLD && current < pool->max_threads) {
        int new_workers = (current * 0.2 > 1) ? current * 0.2 : 1;
        int target = (current + new_workers <= pool->max_threads) ? 
                     current + new_workers : pool->max_threads;

        for (int i = current; i < target; i++) {
            pool->workers[i] = malloc(sizeof(Worker));
            if (!pool->workers[i]) continue;

            pool->workers[i]->id = i;
            pool->workers[i]->pool = pool;
            pool->workers[i]->is_active = true;
            pool->workers[i]->local_queue = deque_create();
            pthread_create(&pool->workers[i]->thread, NULL, worker_routine, pool->workers[i]);
        }
        atomic_store(&pool->current_threads, target);
    }
    // Skalierung nach unten
    else if (load < pool->target_load - SCALING_THRESHOLD && current > pool->min_threads) {
        int reduce = (current * 0.2 > 1) ? current * 0.2 : 1;
        int target = (current - reduce >= pool->min_threads) ? 
                     current - reduce : pool->min_threads;

        for (int i = current - 1; i >= target; i--) {
            pool->workers[i]->is_active = false;
            pthread_join(pool->workers[i]->thread, NULL);
            deque_destroy(pool->workers[i]->local_queue);
            free(pool->workers[i]);
        }
        atomic_store(&pool->current_threads, target);
    }

    pthread_mutex_unlock(&pool->pool_mutex);
}

void threadpool_submit(ThreadPool* pool, void (*function)(void*), void* arg) {
    if (atomic_load(&pool->shutdown)) return;

    Task* task = malloc(sizeof(Task));
    if (!task) return;

    task->function = function;
    task->arg = arg;
    task->next = NULL;

    // Round-Robin Verteilung auf Worker
    static atomic_int next_worker = 0;
    int worker_id = atomic_fetch_add(&next_worker, 1) % atomic_load(&pool->current_threads);
    
    deque_push(pool->workers[worker_id]->local_queue, task);
    
    pthread_mutex_lock(&pool->pool_mutex);
    pthread_cond_broadcast(&pool->pool_cond);
    pthread_mutex_unlock(&pool->pool_mutex);
}

void threadpool_destroy(ThreadPool* pool) {
    if (!pool) return;

    atomic_store(&pool->shutdown, true);
    
    pthread_mutex_lock(&pool->pool_mutex);
    pthread_cond_broadcast(&pool->pool_cond);
    pthread_mutex_unlock(&pool->pool_mutex);

    int current = atomic_load(&pool->current_threads);
    for (int i = 0; i < current; i++) {
        if (pool->workers[i]) {
            pthread_join(pool->workers[i]->thread, NULL);
            deque_destroy(pool->workers[i]->local_queue);
            free(pool->workers[i]);
        }
    }

    free(pool->workers);
    pthread_mutex_destroy(&pool->pool_mutex);
    pthread_cond_destroy(&pool->pool_cond);
    free(pool);
}

// Beispiel zur Verwendung
void example_task(void* arg) {
    int* num = (int*)arg;
    printf("Task %d ausgeführt\n", *num);
    free(num);  // Speicher freigeben
}

int main() {
    // Pool mit 4-16 Threads erstellen
    ThreadPool* pool = threadpool_create(4, 16);
    
    // 100 Tasks einreichen
    for (int i = 0; i < 100; i++) {
        int* num = malloc(sizeof(int));
        *num = i;
        threadpool_submit(pool, example_task, num);
    }
    
    // Kurz warten und dann aufräumen
    sleep(2);
    threadpool_destroy(pool);
    
    return 0;
}