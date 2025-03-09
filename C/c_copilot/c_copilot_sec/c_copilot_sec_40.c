#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LOG_SIZE 1000
#define MAX_LOG_FILES 10

typedef struct {
    char log_file[100];
    FILE *file_ptr;
    pthread_mutex_t lock;
} Logger;

Logger logger;

void initialize_logger(const char *log_file) {
    strncpy(logger.log_file, log_file, sizeof(logger.log_file));
    logger.file_ptr = fopen(logger.log_file, "a");
    if (logger.file_ptr == NULL) {
        perror("Error opening log file");
        exit(1);
    }
    pthread_mutex_init(&logger.lock, NULL);
}

void log_message(const char *message) {
    pthread_mutex_lock(&logger.lock);
    fprintf(logger.file_ptr, "%s\n", message);
    fflush(logger.file_ptr);
    pthread_mutex_unlock(&logger.lock);
}

void close_logger() {
    pthread_mutex_lock(&logger.lock);
    fclose(logger.file_ptr);
    pthread_mutex_unlock(&logger.lock);
    pthread_mutex_destroy(&logger.lock);
}

int main() {
    initialize_logger("/path/to/log/file.log");

    // Log some messages
    log_message("Message 1");
    log_message("Message 2");
    log_message("Message 3");

    close_logger();

    return 0;
}
