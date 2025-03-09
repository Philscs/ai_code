#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

// Struktur zur Verwaltung des Shared Memory
typedef struct {
    int shm_id;
    void *shm_addr;
    size_t size;
    pthread_mutex_t mutex;
    int is_initialized;
} SharedMemory;

// Initialisierung des Shared Memory
int init_shared_memory(SharedMemory *manager, key_t key, size_t size, int permissions) {
    if (!manager) return -1;

    pthread_mutex_lock(&manager->mutex);
    if (manager->is_initialized) {
        pthread_mutex_unlock(&manager->mutex);
        return -1; // Bereits initialisiert
    }

    manager->shm_id = shmget(key, size, IPC_CREAT | permissions);
    if (manager->shm_id < 0) {
        perror("shmget");
        pthread_mutex_unlock(&manager->mutex);
        return -1;
    }

    manager->shm_addr = shmat(manager->shm_id, NULL, 0);
    if (manager->shm_addr == (void *)-1) {
        perror("shmat");
        pthread_mutex_unlock(&manager->mutex);
        return -1;
    }

    manager->size = size;
    manager->is_initialized = 1;
    pthread_mutex_unlock(&manager->mutex);
    return 0;
}

// Schreiben in den Shared Memory (thread-sicher)
int write_shared_memory(SharedMemory *manager, const void *data, size_t size, size_t offset) {
    if (!manager || !data || offset + size > manager->size) return -1;

    pthread_mutex_lock(&manager->mutex);
    if (!manager->is_initialized) {
        pthread_mutex_unlock(&manager->mutex);
        return -1;
    }

    memcpy((char *)manager->shm_addr + offset, data, size);
    pthread_mutex_unlock(&manager->mutex);
    return 0;
}

// Lesen aus dem Shared Memory (thread-sicher)
int read_shared_memory(SharedMemory *manager, void *buffer, size_t size, size_t offset) {
    if (!manager || !buffer || offset + size > manager->size) return -1;

    pthread_mutex_lock(&manager->mutex);
    if (!manager->is_initialized) {
        pthread_mutex_unlock(&manager->mutex);
        return -1;
    }

    memcpy(buffer, (char *)manager->shm_addr + offset, size);
    pthread_mutex_unlock(&manager->mutex);
    return 0;
}

// Freigabe des Shared Memory
int cleanup_shared_memory(SharedMemory *manager) {
    if (!manager) return -1;

    pthread_mutex_lock(&manager->mutex);
    if (!manager->is_initialized) {
        pthread_mutex_unlock(&manager->mutex);
        return -1;
    }

    if (shmdt(manager->shm_addr) < 0) {
        perror("shmdt");
        pthread_mutex_unlock(&manager->mutex);
        return -1;
    }

    if (shmctl(manager->shm_id, IPC_RMID, NULL) < 0) {
        perror("shmctl");
        pthread_mutex_unlock(&manager->mutex);
        return -1;
    }

    manager->is_initialized = 0;
    pthread_mutex_unlock(&manager->mutex);
    return 0;
}

int main() {
    SharedMemory manager = {
        .shm_id = -1,
        .shm_addr = NULL,
        .size = 0,
        .is_initialized = 0
    };
    pthread_mutex_init(&manager.mutex, NULL);

    key_t key = ftok("/tmp", 'R');
    if (key == -1) {
        perror("ftok");
        return EXIT_FAILURE;
    }

    if (init_shared_memory(&manager, key, 1024, 0666) != 0) {
        fprintf(stderr, "Failed to initialize shared memory\n");
        return EXIT_FAILURE;
    }

    const char *message = "Hello, Shared Memory!";
    if (write_shared_memory(&manager, message, strlen(message) + 1, 0) != 0) {
        fprintf(stderr, "Failed to write to shared memory\n");
        cleanup_shared_memory(&manager);
        return EXIT_FAILURE;
    }

    char buffer[1024] = {0};
    if (read_shared_memory(&manager, buffer, sizeof(buffer), 0) != 0) {
        fprintf(stderr, "Failed to read from shared memory\n");
        cleanup_shared_memory(&manager);
        return EXIT_FAILURE;
    }

    printf("Read from shared memory: %s\n", buffer);

    if (cleanup_shared_memory(&manager) != 0) {
        fprintf(stderr, "Failed to clean up shared memory\n");
        return EXIT_FAILURE;
    }

    pthread_mutex_destroy(&manager.mutex);
    return EXIT_SUCCESS;
}
