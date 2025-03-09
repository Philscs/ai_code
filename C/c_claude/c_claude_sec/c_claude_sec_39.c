#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <signal.h>

#define MAX_LOG_MESSAGE 256
#define MAX_RESOURCES 100
#define CRITICAL_THRESHOLD 90.0  // 90% usage threshold

// Resource usage structure
typedef struct {
    char name[50];
    double usage;
    time_t timestamp;
} ResourceUsage;

// Monitor structure
typedef struct {
    ResourceUsage resources[MAX_RESOURCES];
    int resource_count;
    pthread_mutex_t lock;
    FILE* log_file;
    void (*alert_callback)(const char* message);
    int is_running;
} ResourceMonitor;

// Function prototypes
ResourceMonitor* init_monitor(void (*alert_callback)(const char*));
void destroy_monitor(ResourceMonitor* monitor);
int track_resource(ResourceMonitor* monitor, const char* name, double usage);
void log_message(ResourceMonitor* monitor, const char* message);
void check_thresholds(ResourceMonitor* monitor);
void* monitoring_thread(void* arg);
void error_recovery(ResourceMonitor* monitor);

// Initialize the resource monitor
ResourceMonitor* init_monitor(void (*alert_callback)(const char* message)) {
    ResourceMonitor* monitor = (ResourceMonitor*)malloc(sizeof(ResourceMonitor));
    if (!monitor) {
        return NULL;
    }

    // Initialize monitor properties
    monitor->resource_count = 0;
    monitor->is_running = 1;
    monitor->alert_callback = alert_callback;

    // Initialize mutex for thread safety
    if (pthread_mutex_init(&monitor->lock, NULL) != 0) {
        free(monitor);
        return NULL;
    }

    // Open log file
    monitor->log_file = fopen("resource_monitor.log", "a");
    if (!monitor->log_file) {
        pthread_mutex_destroy(&monitor->lock);
        free(monitor);
        return NULL;
    }

    return monitor;
}

// Destroy the resource monitor and clean up resources
void destroy_monitor(ResourceMonitor* monitor) {
    if (!monitor) return;

    monitor->is_running = 0;
    pthread_mutex_destroy(&monitor->lock);
    
    if (monitor->log_file) {
        fclose(monitor->log_file);
    }
    
    free(monitor);
}

// Track resource usage with thread safety
int track_resource(ResourceMonitor* monitor, const char* name, double usage) {
    if (!monitor || !name) return -1;

    pthread_mutex_lock(&monitor->lock);

    // Error recovery if resource count exceeds maximum
    if (monitor->resource_count >= MAX_RESOURCES) {
        log_message(monitor, "Error: Maximum resource limit reached");
        error_recovery(monitor);
        pthread_mutex_unlock(&monitor->lock);
        return -1;
    }

    // Add new resource usage data
    ResourceUsage* resource = &monitor->resources[monitor->resource_count];
    strncpy(resource->name, name, sizeof(resource->name) - 1);
    resource->usage = usage;
    resource->timestamp = time(NULL);
    monitor->resource_count++;

    // Log the tracking
    char log_msg[MAX_LOG_MESSAGE];
    snprintf(log_msg, sizeof(log_msg), "Resource tracked: %s, Usage: %.2f%%", 
             name, usage);
    log_message(monitor, log_msg);

    pthread_mutex_unlock(&monitor->lock);
    return 0;
}

// Logging system implementation
void log_message(ResourceMonitor* monitor, const char* message) {
    if (!monitor || !monitor->log_file || !message) return;

    time_t now = time(NULL);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';  // Remove newline

    pthread_mutex_lock(&monitor->lock);
    fprintf(monitor->log_file, "[%s] %s\n", timestamp, message);
    fflush(monitor->log_file);
    pthread_mutex_unlock(&monitor->lock);
}

// Check resource thresholds and trigger alerts
void check_thresholds(ResourceMonitor* monitor) {
    if (!monitor) return;

    pthread_mutex_lock(&monitor->lock);
    
    for (int i = 0; i < monitor->resource_count; i++) {
        if (monitor->resources[i].usage > CRITICAL_THRESHOLD) {
            char alert_msg[MAX_LOG_MESSAGE];
            snprintf(alert_msg, sizeof(alert_msg), 
                    "ALERT: Resource %s exceeded critical threshold (%.2f%%)",
                    monitor->resources[i].name, monitor->resources[i].usage);
            
            // Trigger alert callback
            if (monitor->alert_callback) {
                monitor->alert_callback(alert_msg);
            }
            
            // Log alert
            log_message(monitor, alert_msg);
        }
    }
    
    pthread_mutex_unlock(&monitor->lock);
}

// Error recovery implementation
void error_recovery(ResourceMonitor* monitor) {
    if (!monitor) return;

    // Log recovery attempt
    log_message(monitor, "Initiating error recovery procedure");

    pthread_mutex_lock(&monitor->lock);

    // Clear old resources
    monitor->resource_count = 0;
    memset(monitor->resources, 0, sizeof(monitor->resources));

    // Reopen log file if needed
    if (!monitor->log_file) {
        monitor->log_file = fopen("resource_monitor.log", "a");
        if (monitor->log_file) {
            log_message(monitor, "Log file successfully reopened");
        }
    }

    pthread_mutex_unlock(&monitor->lock);
}

// Monitoring thread function
void* monitoring_thread(void* arg) {
    ResourceMonitor* monitor = (ResourceMonitor*)arg;
    
    while (monitor->is_running) {
        check_thresholds(monitor);
        sleep(5);  // Check every 5 seconds
    }
    
    return NULL;
}

// Example usage and alert callback
void alert_handler(const char* message) {
    printf("Alert received: %s\n", message);
}

int main() {
    // Initialize monitor with alert callback
    ResourceMonitor* monitor = init_monitor(alert_handler);
    if (!monitor) {
        fprintf(stderr, "Failed to initialize resource monitor\n");
        return 1;
    }

    // Create monitoring thread
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, monitoring_thread, monitor);

    // Example usage
    track_resource(monitor, "CPU", 95.5);
    track_resource(monitor, "Memory", 85.2);
    track_resource(monitor, "Disk", 92.7);

    // Wait for some time to allow monitoring
    sleep(10);

    // Cleanup
    destroy_monitor(monitor);
    pthread_join(thread_id, NULL);

    return 0;
}