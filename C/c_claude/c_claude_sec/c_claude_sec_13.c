#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdatomic.h>

// Task priority levels
typedef enum {
    PRIORITY_LOW,
    PRIORITY_MEDIUM,
    PRIORITY_HIGH
} TaskPriority;

// Task structure
typedef struct Task {
    void (*function)(void*);
    void* arg;
    TaskPriority priority;
    struct Task* next;
} Task;

// Work queue structure
typedef struct WorkQueue {
    Task* head;
    Task* tail;
    size_t size;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} WorkQueue;

// Thread pool structure
typedef struct ThreadPool {
    pthread_t* threads;
    WorkQueue** queues;
    size_t num_threads;
    atomic_bool running;
    size_t max_queue_size;
} ThreadPool;

// Initialize work queue
static WorkQueue* work_queue_init(void) {
    WorkQueue* queue = (WorkQueue*)malloc(sizeof(WorkQueue));
    if (!queue) return NULL;

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;

    if (pthread_mutex_init(&queue->lock, NULL) != 0) {
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->lock);
        free(queue);
        return NULL;
    }

    return queue;
}

// Push task to queue with priority
static bool queue_push(WorkQueue* queue, Task* task, size_t max_size) {
    pthread_mutex_lock(&queue->lock);

    if (queue->size >= max_size) {
        pthread_mutex_unlock(&queue->lock);
        return false;
    }

    if (queue->tail == NULL) {
        queue->head = queue->tail = task;
    } else {
        // Priority-based insertion
        Task* current = queue->head;
        Task* prev = NULL;

        while (current && current->priority >= task->priority) {
            prev = current;
            current = current->next;
        }

        if (prev == NULL) {
            task->next = queue->head;
            queue->head = task;
        } else {
            task->next = current;
            prev->next = task;
            if (current == NULL) {
                queue->tail = task;
            }
        }
    }

    queue->size++;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
    return true;
}

// Pop task from queue
static Task* queue_pop(WorkQueue* queue) {
    pthread_mutex_lock(&queue->lock);

    while (queue->head == NULL) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }

    Task* task = queue->head;
    queue->head = task->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->size--;

    pthread_mutex_unlock(&queue->lock);
    return task;
}

// Work stealing function
static Task* steal_task(ThreadPool* pool, size_t current_thread) {
    for (size_t i = 0; i < pool->num_threads; i++) {
        if (i == current_thread) continue;

        WorkQueue* victim = pool->queues[i];
        pthread_mutex_lock(&victim->lock);

        if (victim->head != NULL) {
            Task* task = victim->head;
            victim->head = task->next;
            if (victim->head == NULL) {
                victim->tail = NULL;
            }
            victim->size--;
            pthread_mutex_unlock(&victim->lock);
            return task;
        }

        pthread_mutex_unlock(&victim->lock);
    }
    return NULL;
}

// Worker thread function
static void* worker(void* arg) {
    ThreadPool* pool = ((void**)arg)[0];
    size_t thread_id = (size_t)((void**)arg)[1];
    free(arg);

    while (atomic_load(&pool->running)) {
        // Try to get task from own queue
        Task* task = NULL;
        WorkQueue* my_queue = pool->queues[thread_id];

        pthread_mutex_lock(&my_queue->lock);
        if (my_queue->head != NULL) {
            task = queue_pop(my_queue);
        }
        pthread_mutex_unlock(&my_queue->lock);

        // If no task found, try work stealing
        if (task == NULL) {
            task = steal_task(pool, thread_id);
        }

        // Execute task if found
        if (task != NULL) {
            task->function(task->arg);
            free(task);
        }
    }

    return NULL;
}

// Initialize thread pool
ThreadPool* thread_pool_init(size_t num_threads, size_t max_queue_size) {
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (!pool) return NULL;

    pool->threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    pool->queues = (WorkQueue**)malloc(num_threads * sizeof(WorkQueue*));
    
    if (!pool->threads || !pool->queues) {
        free(pool->threads);
        free(pool->queues);
        free(pool);
        return NULL;
    }

    pool->num_threads = num_threads;
    pool->max_queue_size = max_queue_size;
    atomic_store(&pool->running, true);

    // Initialize work queues
    for (size_t i = 0; i < num_threads; i++) {
        pool->queues[i] = work_queue_init();
        if (!pool->queues[i]) {
            // Cleanup on failure
            for (size_t j = 0; j < i; j++) {
                pthread_mutex_destroy(&pool->queues[j]->lock);
                pthread_cond_destroy(&pool->queues[j]->not_empty);
                free(pool->queues[j]);
            }
            free(pool->threads);
            free(pool->queues);
            free(pool);
            return NULL;
        }
    }

    // Create worker threads
    for (size_t i = 0; i < num_threads; i++) {
        void** thread_arg = malloc(2 * sizeof(void*));
        thread_arg[0] = pool;
        thread_arg[1] = (void*)i;

        if (pthread_create(&pool->threads[i], NULL, worker, thread_arg) != 0) {
            // Cleanup on failure
            atomic_store(&pool->running, false);
            for (size_t j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            for (size_t j = 0; j < num_threads; j++) {
                pthread_mutex_destroy(&pool->queues[j]->lock);
                pthread_cond_destroy(&pool->queues[j]->not_empty);
                free(pool->queues[j]);
            }
            free(pool->threads);
            free(pool->queues);
            free(pool);
            return NULL;
        }
    }

    return pool;
}

// Submit task to thread pool
bool thread_pool_submit(ThreadPool* pool, void (*function)(void*), void* arg, TaskPriority priority) {
    if (!atomic_load(&pool->running)) return false;

    Task* task = (Task*)malloc(sizeof(Task));
    if (!task) return false;

    task->function = function;
    task->arg = arg;
    task->priority = priority;
    task->next = NULL;

    // Try to submit to shortest queue
    size_t min_size = SIZE_MAX;
    size_t target_queue = 0;

    for (size_t i = 0; i < pool->num_threads; i++) {
        pthread_mutex_lock(&pool->queues[i]->lock);
        if (pool->queues[i]->size < min_size) {
            min_size = pool->queues[i]->size;
            target_queue = i;
        }
        pthread_mutex_unlock(&pool->queues[i]->lock);
    }

    return queue_push(pool->queues[target_queue], task, pool->max_queue_size);
}

// Shutdown thread pool
void thread_pool_shutdown(ThreadPool* pool) {
    if (!pool) return;

    atomic_store(&pool->running, false);

    // Wake up all threads
    for (size_t i = 0; i < pool->num_threads; i++) {
        pthread_mutex_lock(&pool->queues[i]->lock);
        pthread_cond_broadcast(&pool->queues[i]->not_empty);
        pthread_mutex_unlock(&pool->queues[i]->lock);
    }

    // Wait for all threads to finish
    for (size_t i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // Cleanup queues
    for (size_t i = 0; i < pool->num_threads; i++) {
        pthread_mutex_destroy(&pool->queues[i]->lock);
        pthread_cond_destroy(&pool->queues[i]->not_empty);
        
        // Free remaining tasks
        Task* current = pool->queues[i]->head;
        while (current != NULL) {
            Task* next = current->next;
            free(current);
            current = next;
        }
        
        free(pool->queues[i]);
    }

    free(pool->queues);
    free(pool->threads);
    free(pool);
}