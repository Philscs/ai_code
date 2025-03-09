#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#define MAX_SEGMENTS 10
#define MAX_NAME_LENGTH 255

typedef struct {
    char name[MAX_NAME_LENGTH];
    void* address;
    size_t size;
    int permissions;
    pthread_mutex_t mutex;
    int reference_count;
} SharedSegment;

typedef struct {
    SharedSegment segments[MAX_SEGMENTS];
    int segment_count;
    pthread_mutex_t manager_mutex;
} SharedMemoryManager;

// Globale Instanz des Managers
static SharedMemoryManager* manager = NULL;

// Initialisierung des Managers
SharedMemoryManager* init_shared_memory_manager() {
    if (manager != NULL) {
        return manager;
    }

    manager = (SharedMemoryManager*)malloc(sizeof(SharedMemoryManager));
    if (manager == NULL) {
        return NULL;
    }

    manager->segment_count = 0;
    pthread_mutex_init(&manager->manager_mutex, NULL);

    // Signal Handler für Cleanup
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);

    return manager;
}

// Erstellen eines neuen Shared Memory Segments
void* create_shared_segment(const char* name, size_t size, int permissions) {
    if (manager == NULL || name == NULL || size == 0) {
        errno = EINVAL;
        return NULL;
    }

    pthread_mutex_lock(&manager->manager_mutex);

    // Überprüfen ob Segment bereits existiert
    for (int i = 0; i < manager->segment_count; i++) {
        if (strcmp(manager->segments[i].name, name) == 0) {
            pthread_mutex_unlock(&manager->manager_mutex);
            errno = EEXIST;
            return NULL;
        }
    }

    // Überprüfen ob noch Platz ist
    if (manager->segment_count >= MAX_SEGMENTS) {
        pthread_mutex_unlock(&manager->manager_mutex);
        errno = ENOMEM;
        return NULL;
    }

    // Shared Memory Segment erstellen
    int fd = shm_open(name, O_CREAT | O_RDWR, permissions);
    if (fd == -1) {
        pthread_mutex_unlock(&manager->manager_mutex);
        return NULL;
    }

    // Größe festlegen
    if (ftruncate(fd, size) == -1) {
        close(fd);
        shm_unlink(name);
        pthread_mutex_unlock(&manager->manager_mutex);
        return NULL;
    }

    // Memory mappen
    void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        shm_unlink(name);
        pthread_mutex_unlock(&manager->manager_mutex);
        return NULL;
    }

    // Segment in Manager eintragen
    SharedSegment* segment = &manager->segments[manager->segment_count];
    strncpy(segment->name, name, MAX_NAME_LENGTH);
    segment->address = addr;
    segment->size = size;
    segment->permissions = permissions;
    segment->reference_count = 1;
    pthread_mutex_init(&segment->mutex, NULL);

    manager->segment_count++;
    pthread_mutex_unlock(&manager->manager_mutex);

    return addr;
}

// Zugriff auf existierendes Segment
void* attach_shared_segment(const char* name) {
    if (manager == NULL || name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    pthread_mutex_lock(&manager->manager_mutex);

    // Segment suchen
    SharedSegment* segment = NULL;
    for (int i = 0; i < manager->segment_count; i++) {
        if (strcmp(manager->segments[i].name, name) == 0) {
            segment = &manager->segments[i];
            break;
        }
    }

    if (segment == NULL) {
        pthread_mutex_unlock(&manager->manager_mutex);
        errno = ENOENT;
        return NULL;
    }

    // Referenzzähler erhöhen
    segment->reference_count++;
    void* addr = segment->address;
    
    pthread_mutex_unlock(&manager->manager_mutex);
    return addr;
}

// Thread-sicherer Schreibzugriff
int write_to_segment(const char* name, const void* data, size_t size, size_t offset) {
    if (manager == NULL || name == NULL || data == NULL) {
        return -1;
    }

    pthread_mutex_lock(&manager->manager_mutex);
    
    // Segment finden
    SharedSegment* segment = NULL;
    for (int i = 0; i < manager->segment_count; i++) {
        if (strcmp(manager->segments[i].name, name) == 0) {
            segment = &manager->segments[i];
            break;
        }
    }

    if (segment == NULL) {
        pthread_mutex_unlock(&manager->manager_mutex);
        return -1;
    }

    // Größenprüfung
    if (offset + size > segment->size) {
        pthread_mutex_unlock(&manager->manager_mutex);
        return -1;
    }

    // Mutex für das spezifische Segment sperren
    pthread_mutex_lock(&segment->mutex);
    
    // Daten kopieren
    memcpy((char*)segment->address + offset, data, size);
    
    pthread_mutex_unlock(&segment->mutex);
    pthread_mutex_unlock(&manager->manager_mutex);

    return 0;
}

// Segment freigeben
int detach_shared_segment(const char* name) {
    if (manager == NULL || name == NULL) {
        return -1;
    }

    pthread_mutex_lock(&manager->manager_mutex);

    // Segment finden
    SharedSegment* segment = NULL;
    int segment_index = -1;
    for (int i = 0; i < manager->segment_count; i++) {
        if (strcmp(manager->segments[i].name, name) == 0) {
            segment = &manager->segments[i];
            segment_index = i;
            break;
        }
    }

    if (segment == NULL) {
        pthread_mutex_unlock(&manager->manager_mutex);
        return -1;
    }

    // Referenzzähler reduzieren
    segment->reference_count--;

    // Wenn keine Referenzen mehr existieren, Segment komplett entfernen
    if (segment->reference_count == 0) {
        munmap(segment->address, segment->size);
        shm_unlink(segment->name);
        pthread_mutex_destroy(&segment->mutex);

        // Array kompaktieren
        if (segment_index < manager->segment_count - 1) {
            memmove(&manager->segments[segment_index],
                    &manager->segments[segment_index + 1],
                    (manager->segment_count - segment_index - 1) * sizeof(SharedSegment));
        }
        manager->segment_count--;
    }

    pthread_mutex_unlock(&manager->manager_mutex);
    return 0;
}

// Signal Handler für Cleanup
void cleanup_handler(int signum) {
    if (manager == NULL) {
        return;
    }

    pthread_mutex_lock(&manager->manager_mutex);

    // Alle Segmente aufräumen
    for (int i = 0; i < manager->segment_count; i++) {
        SharedSegment* segment = &manager->segments[i];
        munmap(segment->address, segment->size);
        shm_unlink(segment->name);
        pthread_mutex_destroy(&segment->mutex);
    }

    // Manager aufräumen
    pthread_mutex_destroy(&manager->manager_mutex);
    free(manager);
    manager = NULL;

    pthread_mutex_unlock(&manager->manager_mutex);

    // Prozess beenden
    exit(signum);
}

// Beispiel für die Verwendung
int main() {
    // Manager initialisieren
    SharedMemoryManager* mgr = init_shared_memory_manager();
    if (mgr == NULL) {
        fprintf(stderr, "Fehler bei der Initialisierung des Managers\n");
        return 1;
    }

    // Segment erstellen
    const char* test_data = "Test Daten";
    void* segment = create_shared_segment("test_segment", 1024, S_IRUSR | S_IWUSR);
    if (segment == NULL) {
        fprintf(stderr, "Fehler beim Erstellen des Segments\n");
        return 1;
    }

    // Daten schreiben
    if (write_to_segment("test_segment", test_data, strlen(test_data) + 1, 0) != 0) {
        fprintf(stderr, "Fehler beim Schreiben der Daten\n");
        return 1;
    }

    // Segment freigeben
    if (detach_shared_segment("test_segment") != 0) {
        fprintf(stderr, "Fehler beim Freigeben des Segments\n");
        return 1;
    }

    return 0;
}