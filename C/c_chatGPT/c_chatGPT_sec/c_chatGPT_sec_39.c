#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define MAX_LOG_SIZE 1024
#define ALERT_THRESHOLD 80 // Beispiel: 80% Auslastung

// Struktur für die Ressourcenüberwachung
typedef struct ResourceMonitor {
    int totalResources;
    int usedResources;
    pthread_mutex_t lock;
    char log[MAX_LOG_SIZE];
} ResourceMonitor;

// Prototypen
void initResourceMonitor(ResourceMonitor* monitor, int totalResources);
void logMessage(ResourceMonitor* monitor, const char* message);
void alertIfNeeded(ResourceMonitor* monitor);
int allocateResource(ResourceMonitor* monitor, int count);
int releaseResource(ResourceMonitor* monitor, int count);
void cleanupResourceMonitor(ResourceMonitor* monitor);

// Initialisierung des Monitors
void initResourceMonitor(ResourceMonitor* monitor, int totalResources) {
    monitor->totalResources = totalResources;
    monitor->usedResources = 0;
    pthread_mutex_init(&monitor->lock, NULL);
    snprintf(monitor->log, MAX_LOG_SIZE, "[INFO] Resource Monitor initialized with %d resources\n", totalResources);
}

// Logging-System
void logMessage(ResourceMonitor* monitor, const char* message) {
    pthread_mutex_lock(&monitor->lock);
    size_t currentLen = strlen(monitor->log);
    if (currentLen + strlen(message) < MAX_LOG_SIZE) {
        strcat(monitor->log, message);
    } else {
        printf("[WARNING] Log size exceeded. Truncating log.\n");
        memset(monitor->log, 0, MAX_LOG_SIZE);
        snprintf(monitor->log, MAX_LOG_SIZE, "[INFO] Log reset due to size overflow.\n");
    }
    pthread_mutex_unlock(&monitor->lock);
}

// Überprüfung der Ressourcen und Alarmmechanismus
void alertIfNeeded(ResourceMonitor* monitor) {
    int usagePercentage = (monitor->usedResources * 100) / monitor->totalResources;
    if (usagePercentage >= ALERT_THRESHOLD) {
        char alertMessage[128];
        snprintf(alertMessage, sizeof(alertMessage), "[ALERT] Resource usage at %d%%!\n", usagePercentage);
        logMessage(monitor, alertMessage);
        printf("%s", alertMessage);
    }
}

// Ressourcen-Allokation
int allocateResource(ResourceMonitor* monitor, int count) {
    pthread_mutex_lock(&monitor->lock);
    if (monitor->usedResources + count > monitor->totalResources) {
        logMessage(monitor, "[ERROR] Resource allocation failed: Not enough resources\n");
        pthread_mutex_unlock(&monitor->lock);
        return -1;
    }
    monitor->usedResources += count;
    char message[128];
    snprintf(message, sizeof(message), "[INFO] Allocated %d resources. Used: %d/%d\n", count, monitor->usedResources, monitor->totalResources);
    logMessage(monitor, message);
    alertIfNeeded(monitor);
    pthread_mutex_unlock(&monitor->lock);
    return 0;
}

// Ressourcen-Freigabe
int releaseResource(ResourceMonitor* monitor, int count) {
    pthread_mutex_lock(&monitor->lock);
    if (count > monitor->usedResources) {
        logMessage(monitor, "[ERROR] Resource release failed: More resources released than allocated\n");
        pthread_mutex_unlock(&monitor->lock);
        return -1;
    }
    monitor->usedResources -= count;
    char message[128];
    snprintf(message, sizeof(message), "[INFO] Released %d resources. Used: %d/%d\n", count, monitor->usedResources, monitor->totalResources);
    logMessage(monitor, message);
    pthread_mutex_unlock(&monitor->lock);
    return 0;
}

// Bereinigung des Monitors
void cleanupResourceMonitor(ResourceMonitor* monitor) {
    pthread_mutex_destroy(&monitor->lock);
    printf("[INFO] Resource Monitor cleaned up. Final log:\n%s", monitor->log);
}

// Hauptprogramm
int main() {
    ResourceMonitor monitor;
    initResourceMonitor(&monitor, 100);

    allocateResource(&monitor, 30);
    allocateResource(&monitor, 50);
    releaseResource(&monitor, 20);
    allocateResource(&monitor, 50);
    releaseResource(&monitor, 70);

    cleanupResourceMonitor(&monitor);
    return 0;
}
