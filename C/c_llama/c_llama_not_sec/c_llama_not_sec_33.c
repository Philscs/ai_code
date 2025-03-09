#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOCK_FILE "/var/locks/myapp.lock"

typedef enum {
    LOCK_ACQUIRED,
    LOCK_EXPIRED,
    LOCK_FAILED
} lock_status_t;

typedef struct lock_t {
    char *name;
    pid_t owner_pid;
    time_t expiration_time;
    int acquired;
} lock_t;

lock_t locks = {NULL, NULL, 0, 0};

void init_locks() {
    locks.name = malloc(256);
    locks.owner_pid = 0;
    locks.expiration_time = 0;
    locks.acquired = 0;
}

lock_status_t acquire_lock(const char *lock_name) {
    lock_t *new_lock = malloc(sizeof(lock_t));

    new_lock->name = strdup(lock_name);
    new_lock->owner_pid = getpid();
    new_lock->expiration_time = time(NULL) + 60; // 1 minute
    new_lock->acquired = 0;

    int fd = open(LOCK_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return LOCK_FAILED;
    }

    char buffer[256];
    sprintf(buffer, "%s:%d:%d", new_lock->name, new_lock->owner_pid, new_lock->expiration_time);
    write(fd, buffer, strlen(buffer));

    close(fd);

    // check if lock exists
    fd = open(LOCK_FILE, O_RDONLY);
    if (fd < 0) {
        return LOCK_FAILED;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        return LOCK_FAILED;
    }

    if ((st.st_size == strlen(buffer)) && (st.st_mode & S_IWUSR)) {
        return LOCK_ACQUIRED;
    } else {
        return LOCK_EXPIRED;
    }
}

int release_lock(const char *lock_name) {
    int fd = open(LOCK_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return -1;
    }

    char buffer[256];
    sprintf(buffer, "%s:%d:0", lock_name, 0);

    write(fd, buffer, strlen(buffer));

    close(fd);

    return 0;
}

int main() {
    init_locks();

    const char *lock_name = "myapp:123";

    printf("%s acquired\n", lock_name);
    if (acquire_lock(lock_name) == LOCK_ACQUIRED) {
        // Lock erworben
        printf("Lock erworben\n");
        sleep(1); // Warte 1 Sekunde, bevor wir den Lock wieder verlassen
    } else {
        // Lock bereits von jemand anderem erworben
        printf("Lock bereits von jemand anderem erworben\n");
    }

    int ret = release_lock(lock_name);
    if (ret == 0) {
        printf("%s released\n", lock_name);
    } else {
        printf("Fehler beim Verlassen des Locks\n");
    }

    return 0;
}