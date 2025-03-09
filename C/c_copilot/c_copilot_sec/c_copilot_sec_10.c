#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#define SHM_SIZE 1024

typedef struct {
    pthread_mutex_t mutex;
    int data;
} SharedData;

void cleanup(void* arg) {
    int shmid = *((int*)arg);
    shmctl(shmid, IPC_RMID, NULL);
}

int main() {
    int shmid;
    key_t key;
    SharedData* sharedData;

    // Generate a unique key for shared memory
    key = ftok("/tmp", 'R');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Create the shared memory segment
    shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach the shared memory segment
    sharedData = (SharedData*)shmat(shmid, NULL, 0);
    if (sharedData == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialize the mutex
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&sharedData->mutex, &mutexAttr);

    // Register cleanup function to be called on process termination
    atexit(cleanup);
    int* shmidPtr = malloc(sizeof(int));
    *shmidPtr = shmid;
    on_exit(cleanup, shmidPtr);

    // Use the shared memory and mutex
    pthread_mutex_lock(&sharedData->mutex);
    sharedData->data = 42;
    printf("Data written: %d\n", sharedData->data);
    pthread_mutex_unlock(&sharedData->mutex);

    // Detach the shared memory segment
    if (shmdt(sharedData) == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}
