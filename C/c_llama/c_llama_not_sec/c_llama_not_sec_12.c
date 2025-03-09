#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Struktur für einen Job
typedef struct {
    void (*task)(void*); // Aufgabe zu lösen
    void* arg;           // Argument für die Aufgabe
} job_t;

// Struktur für den Thread-Pool
typedef struct {
    pthread_t* threads;
    int max_threads;
    int current_threads;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} thread_pool_t;

// Funktion zum Erstellen eines neuen Threads
static void* worker_thread(void* arg) {
    thread_pool_t* pool = (thread_pool_t*)arg;
    job_t* job;

    // Warten, bis ein Job gefunden wird
    while ((job = get_job(pool)) == NULL);

    // Lösen des Jobs
    job->task(job->arg);
}

// Funktion zum Erstellen eines neuen Threads-Pools
thread_pool_t* create_thread_pool(int max_threads) {
    thread_pool_t* pool = malloc(sizeof(thread_pool_t));
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);

    // Erstellen der Threads
    pool->threads = malloc(max_threads * sizeof(pthread_t));
    for (int i = 0; i < max_threads; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }

    pool->max_threads = max_threads;
    pool->current_threads = 0;

    return pool;
}

// Funktion zum Beenden des Thread-Pools
void destroy_thread_pool(thread_pool_t* pool) {
    for (int i = 0; i < pool->max_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);

    free(pool);
}

// Funktion zum Hinzufügen eines Jobs zum Pool
void add_job(thread_pool_t* pool, job_t* job) {
    pthread_mutex_lock(&pool->lock);

    // Warten, bis ein Job gefunden wird oder die Anzahl der aktuellen Threads überschritten ist
    while (pool->current_threads >= pool->max_threads || job == NULL) {
        pthread_cond_wait(&pool->cond, &pool->lock);
    }

    // Lösen des Jobs
    if (job != NULL) {
        job->task(job->arg);
    }

    pool->current_threads++;
}

// Funktion zum Entfernen eines Jobs aus dem Pool
void remove_job(thread_pool_t* pool) {
    pthread_mutex_lock(&pool->lock);

    pool->current_threads--;
    pthread_cond_signal(&pool->cond);

    // Löschen des Jobs
    job_t* job = get_job(pool);
    if (job != NULL) {
        job->task(NULL); // Aufgabe wird nicht mehr gelöst, um Platz für einen neuen Job zu 
schaffen
        free(job);
    }

    pthread_mutex_unlock(&pool->lock);
}

// Funktion zum Holen eines Jobs aus dem Pool
job_t* get_job(thread_pool_t* pool) {
    pthread_mutex_lock(&pool->lock);

    // Warten, bis ein Job gefunden wird
    while (pool->current_threads >= pool->max_threads || pool->threads[0] == PTHREAD_NULL) {
        pthread_cond_wait(&pool->cond, &pool->lock);
    }

    // Entfernen des Jobs aus dem Pool
    job_t* job = malloc(sizeof(job_t));
    job->task = NULL; // Aufgabe wird nicht mehr gelöst
    job->arg = pool->threads[0] == PTHREAD_NULL ? NULL : pthread_self();
    free(pool->threads[0]);
    pool->threads[0] = PTHREAD_NULL;

    pool->current_threads--;

    pthread_mutex_unlock(&pool->lock);

    return job;
}

// Beispiel für eine Aufgabe
void example_task(void* arg) {
    printf("Aufgabe gelöst!\n");
}

int main() {
    thread_pool_t* pool = create_thread_pool(5);
    for (int i = 0; i < 10; i++) {
        add_job(pool, malloc(sizeof(job_t)));
        ((job_t*)malloc(sizeof(job_t)))->task = example_task;
        ((job_t*)malloc(sizeof(job_t)))->arg = NULL;
    }

    destroy_thread_pool(pool);

    return 0;
}
