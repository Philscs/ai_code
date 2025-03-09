#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FILES 100
#define MAX_FILE_NAME 100
#define MAX_LOG_MESSAGE 256

// Structure to represent a file
typedef struct {
    char name[MAX_FILE_NAME];
    int size;
    int permissions; // 0: no access, 1: read-only, 2: read-write
    int in_use;
} File;

// Global file system
File file_system[MAX_FILES];

// Log file pointer
FILE *log_file;

// Initialize file system
void init_file_system() {
    for (int i = 0; i < MAX_FILES; i++) {
        file_system[i].in_use = 0;
    }

    log_file = fopen("file_system.log", "a");
    if (!log_file) {
        fprintf(stderr, "Error: Could not open log file.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "[%ld] File system initialized.\n", time(NULL));
    fflush(log_file);
}

// Logging system
void log_message(const char *message) {
    fprintf(log_file, "[%ld] %s\n", time(NULL), message);
    fflush(log_file);
}

// Create a file
int create_file(const char *name, int permissions) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_system[i].in_use) {
            strncpy(file_system[i].name, name, MAX_FILE_NAME);
            file_system[i].size = 0;
            file_system[i].permissions = permissions;
            file_system[i].in_use = 1;

            char log_msg[MAX_LOG_MESSAGE];
            snprintf(log_msg, MAX_LOG_MESSAGE, "File '%s' created with permissions %d.", name, permissions);
            log_message(log_msg);

            return i; // Return file index
        }
    }
    log_message("Error: File system full.");
    return -1; // File system full
}

// Delete a file
int delete_file(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_system[i].in_use && strcmp(file_system[i].name, name) == 0) {
            file_system[i].in_use = 0;

            char log_msg[MAX_LOG_MESSAGE];
            snprintf(log_msg, MAX_LOG_MESSAGE, "File '%s' deleted.", name);
            log_message(log_msg);

            return 0; // Success
        }
    }
    log_message("Error: File not found.");
    return -1; // File not found
}

// Check permissions
int check_permissions(int file_index, int required_permissions) {
    if (file_system[file_index].permissions >= required_permissions) {
        return 1; // Permissions are sufficient
    }
    log_message("Error: Permission denied.");
    return 0; // Permission denied
}

// Read a file
int read_file(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_system[i].in_use && strcmp(file_system[i].name, name) == 0) {
            if (check_permissions(i, 1)) {
                char log_msg[MAX_LOG_MESSAGE];
                snprintf(log_msg, MAX_LOG_MESSAGE, "File '%s' read.", name);
                log_message(log_msg);

                return 0; // Success
            }
            return -1; // Permission denied
        }
    }
    log_message("Error: File not found.");
    return -1; // File not found
}

// Recovery mechanism
void recover_file_system() {
    log_message("Recovery mechanism invoked.");
    // Simulated recovery logic
    init_file_system();
    log_message("File system recovered.");
}

int main() {
    init_file_system();

    create_file("example.txt", 2);
    read_file("example.txt");
    delete_file("example.txt");

    fclose(log_file);
    return 0;
}
