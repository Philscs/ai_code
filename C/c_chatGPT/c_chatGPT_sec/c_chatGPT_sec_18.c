#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

// Memory tracking
void *tracked_malloc(size_t size);
void tracked_free(void *ptr);
void check_memory_leaks();

// Signal handling
void handle_signal(int sig);

// Resource usage tracking
void track_resource_usage();

// Privilege separation
void drop_privileges();

// IPC
void secure_ipc();

int main() {
    // Privilege separation
    drop_privileges();

    // Signal handling
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // IPC setup
    secure_ipc();

    // Main loop
    while (1) {
        // Track resource usage
        track_resource_usage();

        // Simulate work
        sleep(1);
    }

    // Check for memory leaks before exiting
    check_memory_leaks();

    return 0;
}

// Memory tracking
typedef struct MemNode {
    void *ptr;
    size_t size;
    struct MemNode *next;
} MemNode;

MemNode *head = NULL;

void *tracked_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    MemNode *node = malloc(sizeof(MemNode));
    node->ptr = ptr;
    node->size = size;
    node->next = head;
    head = node;

    return ptr;
}

void tracked_free(void *ptr) {
    MemNode **current = &head;
    while (*current) {
        if ((*current)->ptr == ptr) {
            MemNode *to_free = *current;
            *current = (*current)->next;
            free(to_free->ptr);
            free(to_free);
            return;
        }
        current = &((*current)->next);
    }
    fprintf(stderr, "Attempted to free untracked memory\n");
}

void check_memory_leaks() {
    MemNode *current = head;
    while (current) {
        fprintf(stderr, "Memory leak detected: %p (%zu bytes)\n", current->ptr, current->size);
        MemNode *to_free = current;
        current = current->next;
        free(to_free->ptr);
        free(to_free);
    }
}

// Signal handling
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("SIGINT received. Cleaning up and exiting...\n");
        check_memory_leaks();
        exit(EXIT_SUCCESS);
    }
}

// Resource usage tracking
void track_resource_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == -1) {
        perror("getrusage");
        return;
    }
    printf("CPU time: User=%ld.%06ld sec, System=%ld.%06ld sec\n",
           usage.ru_utime.tv_sec, usage.ru_utime.tv_usec,
           usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
}

// Privilege separation
void drop_privileges() {
    if (setuid(getuid()) == -1) {
        perror("setuid");
        exit(EXIT_FAILURE);
    }
}

// Secure IPC
void secure_ipc() {
    const char *shm_name = "/secure_ipc";
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(int)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    int *shm_data = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    *shm_data = 42; // Example data
    printf("IPC initialized with value: %d\n", *shm_data);
}
