// logger.h
#ifndef SECURE_LOGGER_H
#define SECURE_LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdatomic.h>

#define MAX_LOG_SIZE 10485760  // 10MB
#define MAX_BACKUP_FILES 5
#define MAX_PATH_LENGTH 256
#define MAX_MSG_LENGTH 1024

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

typedef struct {
    char base_path[MAX_PATH_LENGTH];
    LogLevel min_level;
    pthread_mutex_t mutex;
    atomic_int active_writers;
    atomic_size_t current_size;
    int fd;
} Logger;

Logger* logger_init(const char* base_path, LogLevel min_level);
void logger_free(Logger* logger);
int logger_write(Logger* logger, LogLevel level, const char* message);
int logger_rotate(Logger* logger);

#endif // SECURE_LOGGER_H

static const char* level_strings[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR"
};

static int create_backup(const char* source, int backup_number) {
    char backup_path[MAX_PATH_LENGTH];
    snprintf(backup_path, sizeof(backup_path), "%s.%d", source, backup_number);
    
    if (rename(source, backup_path) != 0) {
        return -1;
    }
    
    return 0;
}

Logger* logger_init(const char* base_path, LogLevel min_level) {
    Logger* logger = (Logger*)malloc(sizeof(Logger));
    if (!logger) {
        return NULL;
    }

    strncpy(logger->base_path, base_path, MAX_PATH_LENGTH - 1);
    logger->base_path[MAX_PATH_LENGTH - 1] = '\0';
    logger->min_level = min_level;
    
    if (pthread_mutex_init(&logger->mutex, NULL) != 0) {
        free(logger);
        return NULL;
    }

    atomic_init(&logger->active_writers, 0);
    atomic_init(&logger->current_size, 0);

    // Open log file with proper permissions
    logger->fd = open(base_path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (logger->fd == -1) {
        pthread_mutex_destroy(&logger->mutex);
        free(logger);
        return NULL;
    }

    // Get current file size
    struct stat st;
    if (fstat(logger->fd, &st) == 0) {
        atomic_store(&logger->current_size, st.st_size);
    }

    return logger;
}

void logger_free(Logger* logger) {
    if (!logger) return;

    // Wait for all writers to finish
    while (atomic_load(&logger->active_writers) > 0) {
        usleep(1000);  // Sleep for 1ms
    }

    close(logger->fd);
    pthread_mutex_destroy(&logger->mutex);
    free(logger);
}

int logger_rotate(Logger* logger) {
    if (!logger) return -1;

    pthread_mutex_lock(&logger->mutex);

    // Wait for active writers
    while (atomic_load(&logger->active_writers) > 0) {
        pthread_mutex_unlock(&logger->mutex);
        usleep(1000);
        pthread_mutex_lock(&logger->mutex);
    }

    // Close current file
    close(logger->fd);

    // Rotate backup files
    for (int i = MAX_BACKUP_FILES - 1; i > 0; i--) {
        create_backup(logger->base_path, i - 1);
    }

    // Open new log file
    logger->fd = open(logger->base_path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (logger->fd == -1) {
        pthread_mutex_unlock(&logger->mutex);
        return -1;
    }

    atomic_store(&logger->current_size, 0);
    pthread_mutex_unlock(&logger->mutex);
    return 0;
}

int logger_write(Logger* logger, LogLevel level, const char* message) {
    if (!logger || level < logger->min_level) return 0;

    time_t now;
    time(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    char log_entry[MAX_MSG_LENGTH];
    int len = snprintf(log_entry, sizeof(log_entry), "[%s] [%s] [PID:%d] %s\n",
                      timestamp, level_strings[level], getpid(), message);

    if (len < 0 || len >= sizeof(log_entry)) {
        return -1;
    }

    atomic_fetch_add(&logger->active_writers, 1);

    // Get file lock
    if (flock(logger->fd, LOCK_EX) == -1) {
        atomic_fetch_sub(&logger->active_writers, 1);
        return -1;
    }

    // Check if rotation is needed
    size_t new_size = atomic_fetch_add(&logger->current_size, len);
    if (new_size + len > MAX_LOG_SIZE) {
        logger_rotate(logger);
    }

    // Write log entry
    ssize_t written = write(logger->fd, log_entry, len);
    
    // Release file lock
    flock(logger->fd, LOCK_UN);

    atomic_fetch_sub(&logger->active_writers, 1);

    return (written == len) ? 0 : -1;
}

int main() {
    Logger* logger = logger_init("/var/log/myapp.log", LOG_INFO);
    if (!logger) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    // Example usage
    logger_write(logger, LOG_INFO, "Application started");
    logger_write(logger, LOG_WARNING, "Resource usage high");
    logger_write(logger, LOG_ERROR, "Failed to connect to database");

    logger_free(logger);
    return 0;
}