#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>

// Logging levels
typedef enum {
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
} LogLevel;

// Memory mapped file structure
typedef struct {
    void* mapped_data;
    size_t file_size;
    int fd;
    char* filename;
    mode_t permissions;
    pthread_mutex_t mutex;
} MMapHandler;

// Logging function
void log_message(LogLevel level, const char* message) {
    time_t now;
    time(&now);
    char* timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';  // Remove newline

    const char* level_str;
    switch (level) {
        case LOG_ERROR:   level_str = "ERROR"; break;
        case LOG_WARNING: level_str = "WARNING"; break;
        case LOG_INFO:    level_str = "INFO"; break;
        case LOG_DEBUG:   level_str = "DEBUG"; break;
        default:          level_str = "UNKNOWN";
    }

    fprintf(stderr, "[%s] %s: %s\n", timestamp, level_str, message);
}

// Initialize memory mapped file handler
MMapHandler* mmap_init(const char* filename, mode_t permissions) {
    MMapHandler* handler = (MMapHandler*)malloc(sizeof(MMapHandler));
    if (!handler) {
        log_message(LOG_ERROR, "Failed to allocate memory for handler");
        return NULL;
    }

    handler->filename = strdup(filename);
    handler->permissions = permissions;
    handler->mapped_data = NULL;
    handler->fd = -1;
    
    if (pthread_mutex_init(&handler->mutex, NULL) != 0) {
        log_message(LOG_ERROR, "Failed to initialize mutex");
        free(handler->filename);
        free(handler);
        return NULL;
    }

    return handler;
}

// Open and map the file
int mmap_open(MMapHandler* handler) {
    if (!handler) {
        log_message(LOG_ERROR, "Invalid handler");
        return -1;
    }

    pthread_mutex_lock(&handler->mutex);

    // Open file with appropriate permissions
    handler->fd = open(handler->filename, O_RDWR | O_CREAT, handler->permissions);
    if (handler->fd == -1) {
        log_message(LOG_ERROR, "Failed to open file");
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    // Get file size
    struct stat sb;
    if (fstat(handler->fd, &sb) == -1) {
        log_message(LOG_ERROR, "Failed to get file stats");
        close(handler->fd);
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }
    handler->file_size = sb.st_size;

    // Map the file into memory
    handler->mapped_data = mmap(NULL, handler->file_size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED, handler->fd, 0);

    if (handler->mapped_data == MAP_FAILED) {
        log_message(LOG_ERROR, "Failed to map file to memory");
        close(handler->fd);
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    // Protect memory from unauthorized access
    if (mprotect(handler->mapped_data, handler->file_size, PROT_READ) == -1) {
        log_message(LOG_ERROR, "Failed to set memory protection");
        munmap(handler->mapped_data, handler->file_size);
        close(handler->fd);
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    pthread_mutex_unlock(&handler->mutex);
    log_message(LOG_INFO, "Successfully mapped file to memory");
    return 0;
}

// Read from mapped memory with bounds checking
ssize_t mmap_read(MMapHandler* handler, void* buffer, size_t size, off_t offset) {
    if (!handler || !buffer) {
        log_message(LOG_ERROR, "Invalid parameters for read operation");
        return -1;
    }

    pthread_mutex_lock(&handler->mutex);

    if (offset + size > handler->file_size) {
        log_message(LOG_ERROR, "Read operation exceeds file bounds");
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    // Temporarily enable write protection for the operation
    if (mprotect(handler->mapped_data, handler->file_size, PROT_READ | PROT_WRITE) == -1) {
        log_message(LOG_ERROR, "Failed to modify memory protection for read");
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    memcpy(buffer, (char*)handler->mapped_data + offset, size);

    // Restore read-only protection
    if (mprotect(handler->mapped_data, handler->file_size, PROT_READ) == -1) {
        log_message(LOG_ERROR, "Failed to restore memory protection after read");
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    pthread_mutex_unlock(&handler->mutex);
    return size;
}

// Write to mapped memory with bounds checking
ssize_t mmap_write(MMapHandler* handler, const void* buffer, size_t size, off_t offset) {
    if (!handler || !buffer) {
        log_message(LOG_ERROR, "Invalid parameters for write operation");
        return -1;
    }

    pthread_mutex_lock(&handler->mutex);

    if (offset + size > handler->file_size) {
        log_message(LOG_ERROR, "Write operation exceeds file bounds");
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    // Enable write protection for the operation
    if (mprotect(handler->mapped_data, handler->file_size, PROT_READ | PROT_WRITE) == -1) {
        log_message(LOG_ERROR, "Failed to modify memory protection for write");
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    memcpy((char*)handler->mapped_data + offset, buffer, size);

    // Sync changes to disk
    if (msync(handler->mapped_data, handler->file_size, MS_SYNC) == -1) {
        log_message(LOG_ERROR, "Failed to sync memory to disk");
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    // Restore read-only protection
    if (mprotect(handler->mapped_data, handler->file_size, PROT_READ) == -1) {
        log_message(LOG_ERROR, "Failed to restore memory protection after write");
        pthread_mutex_unlock(&handler->mutex);
        return -1;
    }

    pthread_mutex_unlock(&handler->mutex);
    return size;
}

// Clean up resources
void mmap_cleanup(MMapHandler* handler) {
    if (!handler) {
        return;
    }

    pthread_mutex_lock(&handler->mutex);

    if (handler->mapped_data != NULL && handler->mapped_data != MAP_FAILED) {
        if (munmap(handler->mapped_data, handler->file_size) == -1) {
            log_message(LOG_ERROR, "Failed to unmap memory");
        }
    }

    if (handler->fd != -1) {
        if (close(handler->fd) == -1) {
            log_message(LOG_ERROR, "Failed to close file descriptor");
        }
    }

    pthread_mutex_unlock(&handler->mutex);
    pthread_mutex_destroy(&handler->mutex);

    free(handler->filename);
    free(handler);
    
    log_message(LOG_INFO, "Successfully cleaned up resources");
}

// Example usage
int main() {
    // Initialize handler with specific permissions
    MMapHandler* handler = mmap_init("/tmp/test.txt", S_IRUSR | S_IWUSR);
    if (!handler) {
        return 1;
    }

    // Open and map the file
    if (mmap_open(handler) != 0) {
        mmap_cleanup(handler);
        return 1;
    }

    // Example write operation
    const char* test_data = "Hello, World!";
    if (mmap_write(handler, test_data, strlen(test_data), 0) == -1) {
        mmap_cleanup(handler);
        return 1;
    }

    // Example read operation
    char buffer[1024];
    if (mmap_read(handler, buffer, strlen(test_data), 0) == -1) {
        mmap_cleanup(handler);
        return 1;
    }

    // Clean up
    mmap_cleanup(handler);
    return 0;
}