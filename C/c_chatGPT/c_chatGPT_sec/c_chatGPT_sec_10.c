#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define SHM_KEY 0x1234
#define SHM_SIZE 1024
#define MUTEX_TIMEOUT 5

// Shared Memory Structure
typedef struct {
    pthread_mutex_t mutex;
    char data[SHM_SIZE - sizeof(pthread_mutex_t)];
} SharedMemory;

int shm_id;
SharedMemory *shm_ptr;

// Cleanup Function
void cleanup(int signum) {
    printf("Cleaning up shared memory...\n");
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }
    exit(0);
}

// Initialize Shared Memory
void init_shared_memory() {
    shm_id = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (SharedMemory *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Initialize Mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    if (pthread_mutex_init(&shm_ptr->mutex, &attr) != 0) {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }
    pthread_mutexattr_destroy(&attr);

    printf("Shared memory and mutex initialized successfully.\n");
}

// Attach to Existing Shared Memory
void attach_shared_memory() {
    shm_id = shmget(SHM_KEY, SHM_SIZE, 0600);
    if (shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (SharedMemory *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    printf("Attached to shared memory successfully.\n");
}

// Write to Shared Memory
void write_to_shared_memory(const char *message) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += MUTEX_TIMEOUT;

    if (pthread_mutex_timedlock(&shm_ptr->mutex, &ts) != 0) {
        perror("pthread_mutex_timedlock");
        return;
    }

    strncpy(shm_ptr->data, message, sizeof(shm_ptr->data) - 1);
    shm_ptr->data[sizeof(shm_ptr->data) - 1] = '\0';
    printf("Data written: %s\n", shm_ptr->data);

    pthread_mutex_unlock(&shm_ptr->mutex);
}

// Read from Shared Memory
void read_from_shared_memory() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += MUTEX_TIMEOUT;

    if (pthread_mutex_timedlock(&shm_ptr->mutex, &ts) != 0) {
        perror("pthread_mutex_timedlock");
        return;
    }

    printf("Data read: %s\n", shm_ptr->data);

    pthread_mutex_unlock(&shm_ptr->mutex);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, cleanup);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <init|write|read> [message]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "init") == 0) {
        init_shared_memory();
    } else if (strcmp(argv[1], "write") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Please provide a message to write.\n");
            return EXIT_FAILURE;
        }
        attach_shared_memory();
        write_to_shared_memory(argv[2]);
    } else if (strcmp(argv[1], "read") == 0) {
        attach_shared_memory();
        read_from_shared_memory();
    } else {
        fprintf(stderr, "Invalid command. Use <init|write|read>.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
