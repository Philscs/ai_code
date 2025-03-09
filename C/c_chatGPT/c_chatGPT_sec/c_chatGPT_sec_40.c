#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <semaphore.h>

#define LOG_FILE "logfile.log"
#define LOG_MAX_SIZE 1024 * 1024 // 1 MB
#define ROTATION_COUNT 5

// Mutex and semaphore for thread and process safety
pthread_mutex_t log_mutex;
sem_t log_semaphore;

// File pointer for logging
FILE *log_file = NULL;

// Function to rotate log files
void rotate_logs() {
    char old_name[256];
    char new_name[256];

    // Remove the oldest log file
    snprintf(old_name, sizeof(old_name), "%s.%d", LOG_FILE, ROTATION_COUNT);
    remove(old_name);

    // Shift log files
    for (int i = ROTATION_COUNT - 1; i >= 0; i--) {
        snprintf(old_name, sizeof(old_name), "%s.%d", LOG_FILE, i);
        snprintf(new_name, sizeof(new_name), "%s.%d", LOG_FILE, i + 1);
        rename(old_name, new_name);
    }

    // Rename the current log file
    snprintf(new_name, sizeof(new_name), "%s.0", LOG_FILE);
    rename(LOG_FILE, new_name);
}

// Open log file safely
void open_log_file() {
    log_file = fopen(LOG_FILE, "a");
    if (!log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
}

// Log a message to the file
void log_message(const char *format, ...) {
    va_list args;

    // Lock for thread and process safety
    pthread_mutex_lock(&log_mutex);
    sem_wait(&log_semaphore);

    // Open log file if not already open
    if (!log_file) {
        open_log_file();
    }

    // Check file size and rotate if necessary
    fseek(log_file, 0, SEEK_END);
    if (ftell(log_file) >= LOG_MAX_SIZE) {
        fclose(log_file);
        rotate_logs();
        open_log_file();
    }

    // Write the message
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file);

    // Unlock
    sem_post(&log_semaphore);
    pthread_mutex_unlock(&log_mutex);
}

// Clean up resources
void cleanup_logger() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_destroy(&log_mutex);
    sem_destroy(&log_semaphore);
}

// Initialize the logger
void init_logger() {
    if (pthread_mutex_init(&log_mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&log_semaphore, 1, 1) != 0) {
        perror("Semaphore initialization failed");
        exit(EXIT_FAILURE);
    }

    open_log_file();
}

int main() {
    init_logger();

    // Example usage
    log_message("This is a test log entry.");
    log_message("Another entry: %d", 42);

    cleanup_logger();
    return 0;
}
