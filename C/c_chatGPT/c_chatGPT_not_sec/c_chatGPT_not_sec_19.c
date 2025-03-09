#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define MAX_TASKS 10

// Struktur zur Repräsentation einer Aufgabe
typedef struct {
    int id;
    int priority;
    int execution_time; // In Millisekunden
    int deadline;       // In Millisekunden
    struct timespec arrival_time;
    struct timespec start_time;
    pthread_t thread;
    sem_t semaphore;
    int resource;
} Task;

Task tasks[MAX_TASKS];
int task_count = 0;

pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t scheduler_mutex = PTHREAD_MUTEX_INITIALIZER;

void* task_function(void* arg) {
    Task* task = (Task*)arg;

    // Warten, bis der Scheduler die Aufgabe freigibt
    sem_wait(&task->semaphore);

    clock_gettime(CLOCK_MONOTONIC, &task->start_time);
    printf("Task %d started\n", task->id);

    // Simulierte Ausführungszeit
    usleep(task->execution_time * 1000);

    printf("Task %d finished\n", task->id);

    return NULL;
}

void add_task(int id, int priority, int execution_time, int deadline, int resource) {
    if (task_count >= MAX_TASKS) {
        printf("Task queue is full!\n");
        return;
    }

    Task* task = &tasks[task_count++];
    task->id = id;
    task->priority = priority;
    task->execution_time = execution_time;
    task->deadline = deadline;
    task->resource = resource;
    clock_gettime(CLOCK_MONOTONIC, &task->arrival_time);

    sem_init(&task->semaphore, 0, 0);
    pthread_create(&task->thread, NULL, task_function, task);
}

void priority_inheritance(Task* task) {
    pthread_mutex_lock(&resource_mutex);
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].resource == task->resource && tasks[i].priority < task->priority) {
            tasks[i].priority = task->priority;
            printf("Task %d inherited priority from Task %d\n", tasks[i].id, task->id);
        }
    }
    pthread_mutex_unlock(&resource_mutex);
}

void scheduler() {
    while (1) {
        pthread_mutex_lock(&scheduler_mutex);
        int highest_priority = -1;
        Task* selected_task = NULL;
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        for (int i = 0; i < task_count; i++) {
            Task* task = &tasks[i];
            if (task->start_time.tv_sec == 0) {
                int time_elapsed = (now.tv_sec - task->arrival_time.tv_sec) * 1000 +
                                   (now.tv_nsec - task->arrival_time.tv_nsec) / 1000000;
                if (time_elapsed > task->deadline) {
                    printf("Task %d missed its deadline!\n", task->id);
                    continue;
                }

                if (task->priority > highest_priority) {
                    highest_priority = task->priority;
                    selected_task = task;
                }
            }
        }

        if (selected_task) {
            priority_inheritance(selected_task);
            sem_post(&selected_task->semaphore);
        }

        pthread_mutex_unlock(&scheduler_mutex);
        usleep(1000); // Scheduler-Zyklus
    }
}

int main() {
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, (void* (*)(void*))scheduler, NULL);

    add_task(1, 2, 500, 1000, 1);
    add_task(2, 1, 300, 700, 1);
    add_task(3, 3, 400, 1500, 2);

    for (int i = 0; i < task_count; i++) {
        pthread_join(tasks[i].thread, NULL);
    }

    return 0;
}
