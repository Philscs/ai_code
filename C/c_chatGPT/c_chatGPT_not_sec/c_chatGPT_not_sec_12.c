#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define INITIAL_THREAD_COUNT 4
#define MAX_THREAD_COUNT 16

typedef struct Task {
    void (*function)(void*);
    void* arg;
} Task;

typedef struct WorkQueue {
    Task* tasks;
    int capacity;
    int front;
    int rear;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} WorkQueue;

typedef struct ThreadPool {
    pthread_t* threads;
    int thread_count;
    WorkQueue** queues;
    bool stop;
} ThreadPool;

// WorkQueue functions
WorkQueue* create_workqueue(int capacity) {
    WorkQueue* queue = (WorkQueue*)malloc(sizeof(WorkQueue));
    queue->tasks = (Task*)malloc(sizeof(Task) * capacity);
    queue->capacity = capacity;
    queue->front = 0;
    queue->rear = -1;
    queue->count = 0;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return queue;
}

void enqueue_task(WorkQueue* queue, Task task) {
    pthread_mutex_lock(&queue->lock);
    if (queue->count < queue->capacity) {
        queue->rear = (queue->rear + 1) % queue->capacity;
        queue->tasks[queue->rear] = task;
        queue->count++;
        pthread_cond_signal(&queue->cond);
    }
    pthread_mutex_unlock(&queue->lock);
}

bool dequeue_task(WorkQueue* queue, Task* task) {
    bool success = false;
    pthread_mutex_lock(&queue->lock);
    while (queue->count == 0) {
        pthread_cond_wait(&queue->cond, &queue->lock);
    }
    if (queue->count > 0) {
        *task = queue->tasks[queue->front];
        queue->front = (queue->front + 1) % queue->capacity;
        queue->count--;
        success = true;
    }
    pthread_mutex_unlock(&queue->lock);
    return success;
}

bool steal_task(WorkQueue* queue, Task* task) {
    bool success = false;
    pthread_mutex_lock(&queue->lock);
    if (queue->count > 0) {
        *task = queue->tasks[queue->front];
        queue->front = (queue->front + 1) % queue->capacity;
        queue->count--;
        success = true;
    }
    pthread_mutex_unlock(&queue->lock);
    return success;
}

// Thread pool functions
ThreadPool* create_threadpool(int thread_count) {
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
    pool->thread_count = thread_count;
    pool->queues = (WorkQueue**)malloc(sizeof(WorkQueue*) * thread_count);
    pool->stop = false;
    for (int i = 0; i < thread_count; i++) {
        pool->queues[i] = create_workqueue(64);
    }
    return pool;
}

void* thread_worker(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    int thread_id = (int)(uintptr_t)pthread_getspecific(pthread_self());

    while (!pool->stop) {
        Task task;
        bool found_task = dequeue_task(pool->queues[thread_id], &task);

        if (!found_task) {
            for (int i = 0; i < pool->thread_count; i++) {
                if (i != thread_id && steal_task(pool->queues[i], &task)) {
                    found_task = true;
                    break;
                }
            }
        }

        if (found_task) {
            task.function(task.arg);
        }
    }
    return NULL;
}

void add_task(ThreadPool* pool, int thread_id, void (*function)(void*), void* arg) {
    Task task = { .function = function, .arg = arg };
    enqueue_task(pool->queues[thread_id], task);
}

void start_threadpool(ThreadPool* pool) {
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_create(&pool->threads[i], NULL, thread_worker, (void*)pool);
    }
}

void stop_threadpool(ThreadPool* pool) {
    pool->stop = true;
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
}

// Example task
void example_task(void* arg) {
    int id = *(int*)arg;
    printf("Task %d is being executed.\n", id);
    free(arg);
}

int main() {
    ThreadPool* pool = create_threadpool(INITIAL_THREAD_COUNT);
    start_threadpool(pool);

    for (int i = 0; i < 20; i++) {
        int* task_id = (int*)malloc(sizeof(int));
        *task_id = i;
        add_task(pool, i % pool->thread_count, example_task, task_id);
    }

    sleep(2);
    stop_threadpool(pool);
    return 0;
}
