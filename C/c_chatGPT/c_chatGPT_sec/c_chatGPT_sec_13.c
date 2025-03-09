#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>

#define MAX_THREADS 8
#define MAX_TASKS 128

// Task Structure
typedef struct {
    void (*function)(void *);
    void *argument;
    int priority;
} Task;

// Task Queue Structure
typedef struct {
    Task tasks[MAX_TASKS];
    atomic_int head;
    atomic_int tail;
    pthread_mutex_t lock;
} TaskQueue;

// Worker Thread Structure
typedef struct {
    pthread_t thread;
    TaskQueue *queue;
    bool is_running;
} Worker;

// Thread Pool Structure
typedef struct {
    Worker workers[MAX_THREADS];
    int num_threads;
    TaskQueue queues[MAX_THREADS];
} ThreadPool;

// Function Declarations
void enqueue_task(TaskQueue *queue, Task task);
bool dequeue_task(TaskQueue *queue, Task *task);
bool steal_task(TaskQueue *queue, Task *task);
void *worker_function(void *arg);
void thread_pool_submit(ThreadPool *pool, Task task);
void thread_pool_init(ThreadPool *pool, int num_threads);
void thread_pool_destroy(ThreadPool *pool);

// Enqueue Task
void enqueue_task(TaskQueue *queue, Task task) {
    pthread_mutex_lock(&queue->lock);
    int tail = atomic_load(&queue->tail);
    if ((tail + 1) % MAX_TASKS == atomic_load(&queue->head)) {
        fprintf(stderr, "Queue is full\n");
    } else {
        queue->tasks[tail] = task;
        atomic_store(&queue->tail, (tail + 1) % MAX_TASKS);
    }
    pthread_mutex_unlock(&queue->lock);
}

// Dequeue Task
bool dequeue_task(TaskQueue *queue, Task *task) {
    pthread_mutex_lock(&queue->lock);
    int head = atomic_load(&queue->head);
    if (head == atomic_load(&queue->tail)) {
        pthread_mutex_unlock(&queue->lock);
        return false;
    } else {
        *task = queue->tasks[head];
        atomic_store(&queue->head, (head + 1) % MAX_TASKS);
        pthread_mutex_unlock(&queue->lock);
        return true;
    }
}

// Steal Task
bool steal_task(TaskQueue *queue, Task *task) {
    pthread_mutex_lock(&queue->lock);
    int head = atomic_load(&queue->head);
    if (head == atomic_load(&queue->tail)) {
        pthread_mutex_unlock(&queue->lock);
        return false;
    } else {
        *task = queue->tasks[head];
        atomic_store(&queue->head, (head + 1) % MAX_TASKS);
        pthread_mutex_unlock(&queue->lock);
        return true;
    }
}

// Worker Function
void *worker_function(void *arg) {
    Worker *worker = (Worker *)arg;
    Task task;

    while (worker->is_running) {
        if (dequeue_task(worker->queue, &task)) {
            task.function(task.argument);
        } else {
            // Attempt to steal work from other queues
            for (int i = 0; i < MAX_THREADS; ++i) {
                if (worker->queue != &worker->queue[i]) {
                    if (steal_task(&worker->queue[i], &task)) {
                        task.function(task.argument);
                        break;
                    }
                }
            }
        }
    }

    return NULL;
}

// Submit Task to Thread Pool
void thread_pool_submit(ThreadPool *pool, Task task) {
    int thread_index = rand() % pool->num_threads;
    enqueue_task(&pool->queues[thread_index], task);
}

// Initialize Thread Pool
void thread_pool_init(ThreadPool *pool, int num_threads) {
    if (num_threads > MAX_THREADS) num_threads = MAX_THREADS;
    pool->num_threads = num_threads;

    for (int i = 0; i < num_threads; ++i) {
        atomic_store(&pool->queues[i].head, 0);
        atomic_store(&pool->queues[i].tail, 0);
        pthread_mutex_init(&pool->queues[i].lock, NULL);

        pool->workers[i].queue = &pool->queues[i];
        pool->workers[i].is_running = true;

        pthread_create(&pool->workers[i].thread, NULL, worker_function, &pool->workers[i]);
    }
}

// Destroy Thread Pool
void thread_pool_destroy(ThreadPool *pool) {
    for (int i = 0; i < pool->num_threads; ++i) {
        pool->workers[i].is_running = false;
        pthread_join(pool->workers[i].thread, NULL);
        pthread_mutex_destroy(&pool->queues[i].lock);
    }
}

// Example Task Function
void example_task(void *arg) {
    int *num = (int *)arg;
    printf("Processing task with number: %d\n", *num);
}

// Main Function
int main() {
    ThreadPool pool;
    thread_pool_init(&pool, 4);

    for (int i = 0; i < 10; ++i) {
        Task task = {example_task, malloc(sizeof(int)), 0};
        *(int *)task.argument = i;
        thread_pool_submit(&pool, task);
    }

    thread_pool_destroy(&pool);
    return 0;
}
