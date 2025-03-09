#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

// Struktur für einen Cache-Eintrag
typedef struct cache_entry {
    char* filepath;           // Pfad zur Datei
    void* data;              // Gecachete Daten
    size_t size;             // Größe der Daten
    time_t last_access;      // Zeitstempel des letzten Zugriffs
    time_t expiry;           // Ablaufzeitpunkt
    struct cache_entry* next; // Nächster Eintrag in der Hash-Tabelle
} cache_entry_t;

// Hauptstruktur des File Cache
typedef struct {
    cache_entry_t** buckets;     // Hash-Tabelle mit Buckets
    size_t bucket_count;         // Anzahl der Buckets
    size_t max_entries;          // Maximale Anzahl von Einträgen
    size_t current_entries;      // Aktuelle Anzahl von Einträgen
    size_t max_memory;          // Maximaler Speicherverbrauch in Bytes
    size_t current_memory;      // Aktueller Speicherverbrauch in Bytes
    pthread_mutex_t lock;       // Mutex für Thread-Safety
    pthread_rwlock_t* bucket_locks; // Array von Read-Write-Locks für die Buckets
} file_cache_t;

// Hash-Funktion für Dateipfade
static size_t hash_function(const char* filepath, size_t bucket_count) {
    size_t hash = 0;
    while (*filepath) {
        hash = (hash * 31) + *filepath++;
    }
    return hash % bucket_count;
}

// Cache initialisieren
file_cache_t* cache_init(size_t bucket_count, size_t max_entries, size_t max_memory) {
    if (bucket_count == 0 || max_entries == 0 || max_memory == 0) {
        errno = EINVAL;
        return NULL;
    }

    file_cache_t* cache = malloc(sizeof(file_cache_t));
    if (!cache) {
        return NULL;
    }

    cache->buckets = calloc(bucket_count, sizeof(cache_entry_t*));
    cache->bucket_locks = calloc(bucket_count, sizeof(pthread_rwlock_t));
    if (!cache->buckets || !cache->bucket_locks) {
        free(cache->buckets);
        free(cache->bucket_locks);
        free(cache);
        return NULL;
    }

    // Initialisiere Locks
    if (pthread_mutex_init(&cache->lock, NULL) != 0) {
        free(cache->buckets);
        free(cache->bucket_locks);
        free(cache);
        return NULL;
    }

    for (size_t i = 0; i < bucket_count; i++) {
        if (pthread_rwlock_init(&cache->bucket_locks[i], NULL) != 0) {
            // Cleanup bei Fehler
            for (size_t j = 0; j < i; j++) {
                pthread_rwlock_destroy(&cache->bucket_locks[j]);
            }
            pthread_mutex_destroy(&cache->lock);
            free(cache->buckets);
            free(cache->bucket_locks);
            free(cache);
            return NULL;
        }
    }

    cache->bucket_count = bucket_count;
    cache->max_entries = max_entries;
    cache->current_entries = 0;
    cache->max_memory = max_memory;
    cache->current_memory = 0;

    return cache;
}

// Cache-Eintrag hinzufügen oder aktualisieren
int cache_put(file_cache_t* cache, const char* filepath, void* data, size_t size, time_t expiry) {
    if (!cache || !filepath || !data || size == 0) {
        errno = EINVAL;
        return -1;
    }

    size_t bucket = hash_function(filepath, cache->bucket_count);
    
    // Prüfe Ressourcenlimits
    pthread_mutex_lock(&cache->lock);
    if (cache->current_entries >= cache->max_entries ||
        cache->current_memory + size > cache->max_memory) {
        pthread_mutex_unlock(&cache->lock);
        errno = ENOMEM;
        return -1;
    }
    pthread_mutex_unlock(&cache->lock);

    // Bucket-Lock für Schreibzugriff
    pthread_rwlock_wrlock(&cache->bucket_locks[bucket]);

    cache_entry_t* entry = cache->buckets[bucket];
    cache_entry_t* prev = NULL;

    // Suche nach existierendem Eintrag
    while (entry) {
        if (strcmp(entry->filepath, filepath) == 0) {
            // Aktualisiere existierenden Eintrag
            void* new_data = malloc(size);
            if (!new_data) {
                pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
                return -1;
            }

            memcpy(new_data, data, size);
            free(entry->data);
            entry->data = new_data;
            entry->size = size;
            entry->last_access = time(NULL);
            entry->expiry = expiry;

            pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }

    // Erstelle neuen Eintrag
    cache_entry_t* new_entry = malloc(sizeof(cache_entry_t));
    if (!new_entry) {
        pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
        return -1;
    }

    new_entry->filepath = strdup(filepath);
    new_entry->data = malloc(size);
    if (!new_entry->filepath || !new_entry->data) {
        free(new_entry->filepath);
        free(new_entry->data);
        free(new_entry);
        pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
        return -1;
    }

    memcpy(new_entry->data, data, size);
    new_entry->size = size;
    new_entry->last_access = time(NULL);
    new_entry->expiry = expiry;
    new_entry->next = NULL;

    // Füge neuen Eintrag in Bucket ein
    if (prev) {
        prev->next = new_entry;
    } else {
        cache->buckets[bucket] = new_entry;
    }

    // Aktualisiere Statistiken
    pthread_mutex_lock(&cache->lock);
    cache->current_entries++;
    cache->current_memory += size;
    pthread_mutex_unlock(&cache->lock);

    pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
    return 0;
}

// Daten aus dem Cache abrufen
void* cache_get(file_cache_t* cache, const char* filepath, size_t* size) {
    if (!cache || !filepath || !size) {
        errno = EINVAL;
        return NULL;
    }

    size_t bucket = hash_function(filepath, cache->bucket_count);
    
    // Bucket-Lock für Lesezugriff
    pthread_rwlock_rdlock(&cache->bucket_locks[bucket]);

    cache_entry_t* entry = cache->buckets[bucket];
    while (entry) {
        if (strcmp(entry->filepath, filepath) == 0) {
            // Prüfe auf Ablauf
            time_t now = time(NULL);
            if (entry->expiry && now > entry->expiry) {
                pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
                errno = EEXIST;
                return NULL;
            }

            // Kopiere Daten
            void* data = malloc(entry->size);
            if (!data) {
                pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
                return NULL;
            }
            memcpy(data, entry->data, entry->size);
            *size = entry->size;

            // Aktualisiere Zugriffszeitpunkt
            entry->last_access = now;

            pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
            return data;
        }
        entry = entry->next;
    }

    pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
    errno = ENOENT;
    return NULL;
}

// Cache-Eintrag entfernen
int cache_remove(file_cache_t* cache, const char* filepath) {
    if (!cache || !filepath) {
        errno = EINVAL;
        return -1;
    }

    size_t bucket = hash_function(filepath, cache->bucket_count);
    
    pthread_rwlock_wrlock(&cache->bucket_locks[bucket]);

    cache_entry_t* entry = cache->buckets[bucket];
    cache_entry_t* prev = NULL;

    while (entry) {
        if (strcmp(entry->filepath, filepath) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                cache->buckets[bucket] = entry->next;
            }

            // Aktualisiere Statistiken
            pthread_mutex_lock(&cache->lock);
            cache->current_entries--;
            cache->current_memory -= entry->size;
            pthread_mutex_unlock(&cache->lock);

            // Cleanup
            free(entry->filepath);
            free(entry->data);
            free(entry);

            pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }

    pthread_rwlock_unlock(&cache->bucket_locks[bucket]);
    errno = ENOENT;
    return -1;
}

// Cache aufräumen (abgelaufene Einträge entfernen)
void cache_cleanup(file_cache_t* cache) {
    if (!cache) {
        return;
    }

    time_t now = time(NULL);

    for (size_t i = 0; i < cache->bucket_count; i++) {
        pthread_rwlock_wrlock(&cache->bucket_locks[i]);

        cache_entry_t* entry = cache->buckets[i];
        cache_entry_t* prev = NULL;

        while (entry) {
            if (entry->expiry && now > entry->expiry) {
                cache_entry_t* to_remove = entry;
                
                if (prev) {
                    prev->next = entry->next;
                } else {
                    cache->buckets[i] = entry->next;
                }

                entry = entry->next;

                // Aktualisiere Statistiken
                pthread_mutex_lock(&cache->lock);
                cache->current_entries--;
                cache->current_memory -= to_remove->size;
                pthread_mutex_unlock(&cache->lock);

                // Cleanup
                free(to_remove->filepath);
                free(to_remove->data);
                free(to_remove);
            } else {
                prev = entry;
                entry = entry->next;
            }
        }

        pthread_rwlock_unlock(&cache->bucket_locks[i]);
    }
}

// Cache zerstören und Ressourcen freigeben
void cache_destroy(file_cache_t* cache) {
    if (!cache) {
        return;
    }

    // Alle Einträge freigeben
    for (size_t i = 0; i < cache->bucket_count; i++) {
        pthread_rwlock_wrlock(&cache->bucket_locks[i]);
        
        cache_entry_t* entry = cache->buckets[i];
        while (entry) {
            cache_entry_t* next = entry->next;
            free(entry->filepath);
            free(entry->data);
            free(entry);
            entry = next;
        }

        pthread_rwlock_unlock(&cache->bucket_locks[i]);
        pthread_rwlock_destroy(&cache->bucket_locks[i]);
    }

    // Cleanup
    pthread_mutex_destroy(&cache->lock);
    free(cache->buckets);
    free(cache->bucket_locks);
    free(cache);
}