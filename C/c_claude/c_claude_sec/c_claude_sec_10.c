#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>

#define SHM_NAME "/secure_shm"
#define SEM_NAME "/secure_sem"
#define SHM_SIZE 4096

typedef struct {
    volatile sig_atomic_t initialized;
    volatile sig_atomic_t process_count;
    char data[SHM_SIZE - sizeof(sig_atomic_t) * 2];
} shared_memory_t;

typedef struct {
    int shm_fd;
    sem_t* mutex;
    shared_memory_t* shared_mem;
    void (*cleanup_handler)(void);
} ipc_context_t;

static ipc_context_t* g_context = NULL;

// Cleanup-Handler für die Prozessbeendigung
void cleanup_handler(void) {
    if (g_context) {
        if (g_context->shared_mem) {
            // Atomare Verringerung der Prozesszählung
            __sync_fetch_and_sub(&g_context->shared_mem->process_count, 1);
            
            // Wenn letzter Prozess, dann aufräumen
            if (g_context->shared_mem->process_count == 0) {
                munmap(g_context->shared_mem, SHM_SIZE);
                shm_unlink(SHM_NAME);
                sem_close(g_context->mutex);
                sem_unlink(SEM_NAME);
            }
        }
        free(g_context);
        g_context = NULL;
    }
}

// Signal-Handler
void signal_handler(int signo) {
    cleanup_handler();
    exit(EXIT_SUCCESS);
}

// Initialisierung des IPC-Kontexts
ipc_context_t* init_ipc(void) {
    ipc_context_t* context = calloc(1, sizeof(ipc_context_t));
    if (!context) {
        return NULL;
    }

    // Registriere Signal-Handler
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Erstelle/Öffne Shared Memory
    context->shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0660);
    if (context->shm_fd == -1) {
        free(context);
        return NULL;
    }

    // Setze Größe des Shared Memory
    if (ftruncate(context->shm_fd, SHM_SIZE) == -1) {
        close(context->shm_fd);
        shm_unlink(SHM_NAME);
        free(context);
        return NULL;
    }

    // Mappe Shared Memory
    context->shared_mem = mmap(NULL, SHM_SIZE, 
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, context->shm_fd, 0);
    if (context->shared_mem == MAP_FAILED) {
        close(context->shm_fd);
        shm_unlink(SHM_NAME);
        free(context);
        return NULL;
    }

    // Initialisiere Mutex
    context->mutex = sem_open(SEM_NAME, O_CREAT, 0660, 1);
    if (context->mutex == SEM_FAILED) {
        munmap(context->shared_mem, SHM_SIZE);
        close(context->shm_fd);
        shm_unlink(SHM_NAME);
        free(context);
        return NULL;
    }

    // Setze globalen Kontext und Cleanup-Handler
    g_context = context;
    context->cleanup_handler = cleanup_handler;
    atexit(cleanup_handler);

    // Erhöhe Prozesszähler atomar
    __sync_fetch_and_add(&context->shared_mem->process_count, 1);

    return context;
}

// Schreiben in Shared Memory mit Timeout
int write_to_shm(ipc_context_t* context, const char* data, size_t len, 
                 struct timespec* timeout) {
    if (!context || !data || len >= SHM_SIZE - sizeof(sig_atomic_t) * 2) {
        return -1;
    }

    // Versuche Mutex zu lokken mit Timeout (Deadlock-Prävention)
    if (sem_timedwait(context->mutex, timeout) == -1) {
        if (errno == ETIMEDOUT) {
            return -2;  // Timeout
        }
        return -1;
    }

    // Kopiere Daten in Shared Memory
    memcpy(context->shared_mem->data, data, len);
    
    // Gebe Mutex frei
    sem_post(context->mutex);
    return 0;
}

// Lesen aus Shared Memory mit Timeout
int read_from_shm(ipc_context_t* context, char* buffer, size_t len,
                  struct timespec* timeout) {
    if (!context || !buffer || len >= SHM_SIZE - sizeof(sig_atomic_t) * 2) {
        return -1;
    }

    // Versuche Mutex zu lokken mit Timeout (Deadlock-Prävention)
    if (sem_timedwait(context->mutex, timeout) == -1) {
        if (errno == ETIMEDOUT) {
            return -2;  // Timeout
        }
        return -1;
    }

    // Kopiere Daten aus Shared Memory
    memcpy(buffer, context->shared_mem->data, len);
    
    // Gebe Mutex frei
    sem_post(context->mutex);
    return 0;
}

// Beispiel Verwendung
int main() {
    ipc_context_t* context = init_ipc();
    if (!context) {
        fprintf(stderr, "Fehler bei der IPC-Initialisierung\n");
        return EXIT_FAILURE;
    }

    // Setze Timeout für 5 Sekunden
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 5;

    // Beispiel: Schreiben in Shared Memory
    const char* test_data = "Test Nachricht";
    if (write_to_shm(context, test_data, strlen(test_data), &timeout) != 0) {
        fprintf(stderr, "Fehler beim Schreiben\n");
        return EXIT_FAILURE;
    }

    // Beispiel: Lesen aus Shared Memory
    char buffer[256] = {0};
    if (read_from_shm(context, buffer, sizeof(buffer), &timeout) != 0) {
        fprintf(stderr, "Fehler beim Lesen\n");
        return EXIT_FAILURE;
    }

    printf("Gelesene Daten: %s\n", buffer);

    return EXIT_SUCCESS;
}