#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define MAX_PROCESSES 100
#define LOG_FILE "scheduler.log"

// Process Structure
typedef struct {
    int pid;
    int priority;
    int resources;
    int is_active;
} Process;

// Scheduler Data Structure
typedef struct {
    Process processes[MAX_PROCESSES];
    int count;
    pthread_mutex_t lock;
} Scheduler;

Scheduler scheduler = {
    .count = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER
};

// Function to log messages
void log_message(const char *message) {
    FILE *logfile = fopen(LOG_FILE, "a");
    if (logfile) {
        time_t now = time(NULL);
        fprintf(logfile, "%s: %s\n", ctime(&now), message);
        fclose(logfile);
    }
}

// Add a new process
int add_process(int priority, int resources) {
    pthread_mutex_lock(&scheduler.lock);
    if (scheduler.count >= MAX_PROCESSES) {
        log_message("Error: Max processes reached.");
        pthread_mutex_unlock(&scheduler.lock);
        return -1;
    }

    Process new_process = {
        .pid = scheduler.count + 1,
        .priority = priority,
        .resources = resources,
        .is_active = 1
    };
    scheduler.processes[scheduler.count++] = new_process;

    char log_msg[100];
    snprintf(log_msg, sizeof(log_msg), "Process %d added with priority %d and resources %d.", new_process.pid, priority, resources);
    log_message(log_msg);

    pthread_mutex_unlock(&scheduler.lock);
    return new_process.pid;
}

// Remove a process
int remove_process(int pid) {
    pthread_mutex_lock(&scheduler.lock);
    for (int i = 0; i < scheduler.count; i++) {
        if (scheduler.processes[i].pid == pid) {
            scheduler.processes[i].is_active = 0;

            char log_msg[100];
            snprintf(log_msg, sizeof(log_msg), "Process %d removed.", pid);
            log_message(log_msg);

            pthread_mutex_unlock(&scheduler.lock);
            return 0;
        }
    }

    log_message("Error: Process not found.");
    pthread_mutex_unlock(&scheduler.lock);
    return -1;
}

// Schedule processes by priority
void schedule_processes() {
    pthread_mutex_lock(&scheduler.lock);

    // Sort processes by priority
    for (int i = 0; i < scheduler.count - 1; i++) {
        for (int j = i + 1; j < scheduler.count; j++) {
            if (scheduler.processes[j].priority > scheduler.processes[i].priority) {
                Process temp = scheduler.processes[i];
                scheduler.processes[i] = scheduler.processes[j];
                scheduler.processes[j] = temp;
            }
        }
    }

    // Display scheduled processes
    printf("Scheduled Processes:\n");
    for (int i = 0; i < scheduler.count; i++) {
        if (scheduler.processes[i].is_active) {
            printf("PID: %d, Priority: %d, Resources: %d\n", 
                scheduler.processes[i].pid, 
                scheduler.processes[i].priority, 
                scheduler.processes[i].resources);
        }
    }

    log_message("Processes scheduled.");
    pthread_mutex_unlock(&scheduler.lock);
}

// Main function for demonstration
int main() {
    log_message("Scheduler started.");

    add_process(5, 10);
    add_process(8, 20);
    add_process(3, 15);

    schedule_processes();

    remove_process(2);

    schedule_processes();

    log_message("Scheduler terminated.");
    return 0;
}
