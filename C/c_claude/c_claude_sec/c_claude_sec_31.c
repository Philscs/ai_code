#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define MAX_PROCESSES 100
#define MAX_LOG_MESSAGE 256
#define MAX_RESOURCES 10

// Process States
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} ProcessState;

// Process Structure
typedef struct {
    int pid;
    int priority;
    ProcessState state;
    int resources[MAX_RESOURCES];
    time_t creation_time;
} Process;

// Scheduler Structure
typedef struct {
    Process processes[MAX_PROCESSES];
    int process_count;
    pthread_mutex_t scheduler_mutex;
    FILE* log_file;
} Scheduler;

// Global scheduler instance
Scheduler scheduler;

// Logging function with timestamp
void log_message(const char* message) {
    time_t now;
    char timestamp[26];
    char log_buffer[MAX_LOG_MESSAGE];
    
    time(&now);
    ctime_r(&now, timestamp);
    timestamp[24] = '\0'; // Remove newline
    
    pthread_mutex_lock(&scheduler.scheduler_mutex);
    
    snprintf(log_buffer, MAX_LOG_MESSAGE, "[%s] %s\n", timestamp, message);
    fprintf(scheduler.log_file, "%s", log_buffer);
    fflush(scheduler.log_file);
    
    pthread_mutex_unlock(&scheduler.scheduler_mutex);
}

// Initialize the scheduler
int init_scheduler() {
    memset(&scheduler, 0, sizeof(Scheduler));
    
    // Initialize mutex
    if (pthread_mutex_init(&scheduler.scheduler_mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        return -1;
    }
    
    // Open log file
    scheduler.log_file = fopen("scheduler.log", "a");
    if (scheduler.log_file == NULL) {
        perror("Log file opening failed");
        return -1;
    }
    
    log_message("Scheduler initialized");
    return 0;
}

// Add a new process
int add_process(int priority) {
    pthread_mutex_lock(&scheduler.scheduler_mutex);
    
    if (scheduler.process_count >= MAX_PROCESSES) {
        pthread_mutex_unlock(&scheduler.scheduler_mutex);
        log_message("Error: Maximum process limit reached");
        return -1;
    }
    
    Process* new_process = &scheduler.processes[scheduler.process_count];
    new_process->pid = scheduler.process_count;
    new_process->priority = priority;
    new_process->state = READY;
    time(&new_process->creation_time);
    memset(new_process->resources, 0, sizeof(new_process->resources));
    
    scheduler.process_count++;
    
    char log_msg[MAX_LOG_MESSAGE];
    snprintf(log_msg, MAX_LOG_MESSAGE, "Process added: PID=%d, Priority=%d", 
             new_process->pid, priority);
    log_message(log_msg);
    
    pthread_mutex_unlock(&scheduler.scheduler_mutex);
    return new_process->pid;
}

// Allocate resources to a process
int allocate_resource(int pid, int resource_id, int amount) {
    if (pid >= scheduler.process_count || resource_id >= MAX_RESOURCES || amount < 0) {
        log_message("Invalid resource allocation request");
        return -1;
    }
    
    pthread_mutex_lock(&scheduler.scheduler_mutex);
    
    Process* process = &scheduler.processes[pid];
    process->resources[resource_id] += amount;
    
    char log_msg[MAX_LOG_MESSAGE];
    snprintf(log_msg, MAX_LOG_MESSAGE, 
             "Resource allocated: PID=%d, ResourceID=%d, Amount=%d",
             pid, resource_id, amount);
    log_message(log_msg);
    
    pthread_mutex_unlock(&scheduler.scheduler_mutex);
    return 0;
}

// Schedule next process based on priority
int schedule_next_process() {
    pthread_mutex_lock(&scheduler.scheduler_mutex);
    
    int highest_priority = -1;
    int selected_pid = -1;
    
    // Find highest priority READY process
    for (int i = 0; i < scheduler.process_count; i++) {
        if (scheduler.processes[i].state == READY &&
            scheduler.processes[i].priority > highest_priority) {
            highest_priority = scheduler.processes[i].priority;
            selected_pid = i;
        }
    }
    
    if (selected_pid != -1) {
        scheduler.processes[selected_pid].state = RUNNING;
        
        char log_msg[MAX_LOG_MESSAGE];
        snprintf(log_msg, MAX_LOG_MESSAGE, 
                 "Process scheduled: PID=%d, Priority=%d",
                 selected_pid, highest_priority);
        log_message(log_msg);
    }
    
    pthread_mutex_unlock(&scheduler.scheduler_mutex);
    return selected_pid;
}

// Error recovery function
void handle_process_error(int pid, const char* error_message) {
    pthread_mutex_lock(&scheduler.scheduler_mutex);
    
    if (pid < scheduler.process_count) {
        Process* process = &scheduler.processes[pid];
        process->state = BLOCKED;
        
        char log_msg[MAX_LOG_MESSAGE];
        snprintf(log_msg, MAX_LOG_MESSAGE, 
                 "Process error: PID=%d, Error=%s",
                 pid, error_message);
        log_message(log_msg);
        
        // Attempt recovery
        process->state = READY;
        log_message("Recovery attempt initiated");
    }
    
    pthread_mutex_unlock(&scheduler.scheduler_mutex);
}

// Cleanup and shutdown
void cleanup_scheduler() {
    pthread_mutex_lock(&scheduler.scheduler_mutex);
    
    if (scheduler.log_file) {
        log_message("Scheduler shutting down");
        fclose(scheduler.log_file);
    }
    
    pthread_mutex_unlock(&scheduler.scheduler_mutex);
    pthread_mutex_destroy(&scheduler.scheduler_mutex);
}

// Example usage
int main() {
    if (init_scheduler() != 0) {
        fprintf(stderr, "Failed to initialize scheduler\n");
        return 1;
    }
    
    // Add some test processes
    int pid1 = add_process(5);  // High priority
    int pid2 = add_process(3);  // Medium priority
    int pid3 = add_process(1);  // Low priority
    
    // Allocate resources
    allocate_resource(pid1, 0, 2);
    allocate_resource(pid2, 1, 1);
    
    // Test scheduling
    int next_pid = schedule_next_process();
    printf("Next scheduled process: PID=%d\n", next_pid);
    
    // Test error recovery
    handle_process_error(pid2, "Resource allocation failed");
    
    // Cleanup
    cleanup_scheduler();
    return 0;
}