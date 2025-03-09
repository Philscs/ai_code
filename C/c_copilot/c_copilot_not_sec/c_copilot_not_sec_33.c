#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#define NUM_LOCKS 10

typedef struct {
    pthread_mutex_t mutex;
    bool is_locked;
} Lock;

Lock locks[NUM_LOCKS];

void init_locks() {
    for (int i = 0; i < NUM_LOCKS; i++) {
        pthread_mutex_init(&locks[i].mutex, NULL);
        locks[i].is_locked = false;
    }
}

void lock(int lock_id) {
    pthread_mutex_lock(&locks[lock_id].mutex);
    locks[lock_id].is_locked = true;
}

void unlock(int lock_id) {
    locks[lock_id].is_locked = false;
    pthread_mutex_unlock(&locks[lock_id].mutex);
}

bool is_locked(int lock_id) {
    return locks[lock_id].is_locked;
}

int main() {
    init_locks();

    // Example usage
    int lock_id = 0;
    lock(lock_id);
    printf("Lock %d is locked: %s\n", lock_id, is_locked(lock_id) ? "true" : "false");
    unlock(lock_id);
    printf("Lock %d is locked: %s\n", lock_id, is_locked(lock_id) ? "true" : "false");

    return 0;
}
