#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    int resource;
    pthread_mutex_t mutex;
    FILE* logFile;
} ResourceMonitor;

void initResourceMonitor(ResourceMonitor* monitor, const char* logFileName) {
    monitor->resource = 0;
    pthread_mutex_init(&monitor->mutex, NULL);
    monitor->logFile = fopen(logFileName, "a");
    if (monitor->logFile == NULL) {
        perror("Failed to open log file");
        exit(1);
    }
}

void destroyResourceMonitor(ResourceMonitor* monitor) {
    pthread_mutex_destroy(&monitor->mutex);
    fclose(monitor->logFile);
}

void logMessage(ResourceMonitor* monitor, const char* message) {
    pthread_mutex_lock(&monitor->mutex);
    fprintf(monitor->logFile, "%s\n", message);
    pthread_mutex_unlock(&monitor->mutex);
}

void incrementResource(ResourceMonitor* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    monitor->resource++;
    pthread_mutex_unlock(&monitor->mutex);
}

void decrementResource(ResourceMonitor* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    if (monitor->resource > 0) {
        monitor->resource--;
    } else {
        logMessage(monitor, "Error: Resource count already zero");
    }
    pthread_mutex_unlock(&monitor->mutex);
}

int getResourceCount(ResourceMonitor* monitor) {
    int count;
    pthread_mutex_lock(&monitor->mutex);
    count = monitor->resource;
    pthread_mutex_unlock(&monitor->mutex);
    return count;
}

void* resourceMonitorThread(void* arg) {
    ResourceMonitor* monitor = (ResourceMonitor*)arg;
    while (1) {
        int count = getResourceCount(monitor);
        if (count > 10) {
            logMessage(monitor, "Alert: Resource count exceeds threshold");
            // Trigger alert mechanism here
        }
        // Sleep for some time
        usleep(1000000); // 1 second
    }
    return NULL;
}

int main() {
    ResourceMonitor monitor;
    initResourceMonitor(&monitor, "resource_monitor.log");

    // Create a thread for monitoring resource count
    pthread_t monitorThread;
    pthread_create(&monitorThread, NULL, resourceMonitorThread, &monitor);

    // Perform some operations on the resource
    incrementResource(&monitor);
    incrementResource(&monitor);
    decrementResource(&monitor);

    // Wait for the monitor thread to finish
    pthread_join(monitorThread, NULL);

    destroyResourceMonitor(&monitor);
    return 0;
}
