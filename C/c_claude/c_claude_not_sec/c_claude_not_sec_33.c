#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_RESOURCES 100
#define MAX_CLIENTS 50
#define LOCK_TIMEOUT 30  // Timeout in Sekunden

// Struktur für einen Lock
typedef struct {
    char resource_id[64];
    int owner_id;
    time_t acquisition_time;
    bool is_locked;
    pthread_mutex_t mutex;
} DistributedLock;

// Struktur für den Lock-Manager
typedef struct {
    DistributedLock locks[MAX_RESOURCES];
    int num_locks;
    pthread_mutex_t global_mutex;
} LockManager;

// Initialisierung des Lock-Managers
LockManager* init_lock_manager() {
    LockManager* manager = (LockManager*)malloc(sizeof(LockManager));
    manager->num_locks = 0;
    pthread_mutex_init(&manager->global_mutex, NULL);
    
    for (int i = 0; i < MAX_RESOURCES; i++) {
        manager->locks[i].is_locked = false;
        manager->locks[i].owner_id = -1;
        pthread_mutex_init(&manager->locks[i].mutex, NULL);
    }
    
    return manager;
}

// Funktion zum Anfordern eines Locks
int acquire_lock(LockManager* manager, const char* resource_id, int client_id) {
    pthread_mutex_lock(&manager->global_mutex);
    
    // Suche nach existierendem Lock
    int lock_index = -1;
    for (int i = 0; i < manager->num_locks; i++) {
        if (strcmp(manager->locks[i].resource_id, resource_id) == 0) {
            lock_index = i;
            break;
        }
    }
    
    // Wenn Lock nicht existiert, erstelle einen neuen
    if (lock_index == -1) {
        if (manager->num_locks >= MAX_RESOURCES) {
            pthread_mutex_unlock(&manager->global_mutex);
            return -1; // Maximale Anzahl an Ressourcen erreicht
        }
        lock_index = manager->num_locks++;
        strncpy(manager->locks[lock_index].resource_id, resource_id, 63);
        manager->locks[lock_index].resource_id[63] = '\0';
    }
    
    DistributedLock* lock = &manager->locks[lock_index];
    
    // Überprüfe, ob der Lock verfügbar ist
    if (lock->is_locked) {
        // Überprüfe Timeout
        time_t current_time = time(NULL);
        if (current_time - lock->acquisition_time > LOCK_TIMEOUT) {
            // Lock ist abgelaufen
            lock->is_locked = false;
        } else {
            pthread_mutex_unlock(&manager->global_mutex);
            return 0; // Lock ist nicht verfügbar
        }
    }
    
    // Vergebe Lock
    lock->is_locked = true;
    lock->owner_id = client_id;
    lock->acquisition_time = time(NULL);
    
    pthread_mutex_unlock(&manager->global_mutex);
    return 1; // Lock erfolgreich erworben
}

// Funktion zum Freigeben eines Locks
int release_lock(LockManager* manager, const char* resource_id, int client_id) {
    pthread_mutex_lock(&manager->global_mutex);
    
    // Suche nach dem Lock
    for (int i = 0; i < manager->num_locks; i++) {
        if (strcmp(manager->locks[i].resource_id, resource_id) == 0) {
            if (manager->locks[i].owner_id == client_id) {
                manager->locks[i].is_locked = false;
                manager->locks[i].owner_id = -1;
                pthread_mutex_unlock(&manager->global_mutex);
                return 1; // Lock erfolgreich freigegeben
            }
            break;
        }
    }
    
    pthread_mutex_unlock(&manager->global_mutex);
    return 0; // Lock konnte nicht freigegeben werden
}

// Aufräumfunktion für den Lock-Manager
void cleanup_lock_manager(LockManager* manager) {
    for (int i = 0; i < MAX_RESOURCES; i++) {
        pthread_mutex_destroy(&manager->locks[i].mutex);
    }
    pthread_mutex_destroy(&manager->global_mutex);
    free(manager);
}

// Beispiel für die Verwendung
int main() {
    LockManager* manager = init_lock_manager();
    
    // Beispiel für Client 1, der einen Lock anfordert
    printf("Client 1 versucht Lock für 'resource1' zu erwerben...\n");
    if (acquire_lock(manager, "resource1", 1)) {
        printf("Client 1 hat Lock für 'resource1' erworben\n");
        
        // Simuliere Arbeit mit der Ressource
        sleep(2);
        
        // Gebe Lock frei
        if (release_lock(manager, "resource1", 1)) {
            printf("Client 1 hat Lock für 'resource1' freigegeben\n");
        }
    }
    
    // Beispiel für Client 2, der denselben Lock anfordert
    printf("Client 2 versucht Lock für 'resource1' zu erwerben...\n");
    if (acquire_lock(manager, "resource1", 2)) {
        printf("Client 2 hat Lock für 'resource1' erworben\n");
        release_lock(manager, "resource1", 2);
    }
    
    cleanup_lock_manager(manager);
    return 0;
}