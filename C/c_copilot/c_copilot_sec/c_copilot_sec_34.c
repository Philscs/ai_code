#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define SHM_SIZE 1024

typedef struct {
    int fd;
    void* addr;
    pthread_mutex_t mutex;
} SharedMemoryManager;

SharedMemoryManager* create_shared_memory_manager(const char* name) {
    SharedMemoryManager* manager = malloc(sizeof(SharedMemoryManager));
    if (manager == NULL) {
        perror("Failed to allocate memory for SharedMemoryManager");
        return NULL;
    }

    manager->fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (manager->fd == -1) {
        perror("Failed to create shared memory");
        free(manager);
        return NULL;
    }

    if (ftruncate(manager->fd, SHM_SIZE) == -1) {
        perror("Failed to set shared memory size");
        close(manager->fd);
        free(manager);
        return NULL;
    }

    manager->addr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, manager->fd, 0);
    if (manager->addr == MAP_FAILED) {
        perror("Failed to map shared memory");
        close(manager->fd);
        free(manager);
        return NULL;
    }

    if (pthread_mutex_init(&manager->mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        munmap(manager->addr, SHM_SIZE);
        close(manager->fd);
        free(manager);
        return NULL;
    }

    return manager;
}

void destroy_shared_memory_manager(SharedMemoryManager* manager) {
    if (manager == NULL) {
        return;
    }

    pthread_mutex_destroy(&manager->mutex);
    munmap(manager->addr, SHM_SIZE);
    close(manager->fd);
    free(manager);
}

void* read_from_shared_memory(SharedMemoryManager* manager) {
    pthread_mutex_lock(&manager->mutex);

    // Read data from shared memory
    void* data = malloc(SHM_SIZE);
    if (data == NULL) {
        perror("Failed to allocate memory for reading from shared memory");
        pthread_mutex_unlock(&manager->mutex);
        return NULL;
    }

    memcpy(data, manager->addr, SHM_SIZE);

    pthread_mutex_unlock(&manager->mutex);

    return data;
}

int write_to_shared_memory(SharedMemoryManager* manager, const void* data, size_t size) {
    if (size > SHM_SIZE) {
        perror("Data size exceeds shared memory size");
        return -1;
    }

    pthread_mutex_lock(&manager->mutex);

    // Write data to shared memory
    memcpy(manager->addr, data, size);

    pthread_mutex_unlock(&manager->mutex);

    return 0;
}

int main() {
    SharedMemoryManager* manager = create_shared_memory_manager("/my_shared_memory");
    if (manager == NULL) {
        return 1;
    }

    // Example usage
    const char* message = "Hello, Shared Memory!";
    if (write_to_shared_memory(manager, message, strlen(message) + 1) != 0) {
        destroy_shared_memory_manager(manager);
        return 1;
    }

    void* data = read_from_shared_memory(manager);
    if (data != NULL) {
        printf("Data read from shared memory: %s\n", (char*)data);
        free(data);
    }

    destroy_shared_memory_manager(manager);

    return 0;
}
