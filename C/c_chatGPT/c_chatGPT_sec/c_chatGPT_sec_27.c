#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define CACHE_SIZE 5 // Maximale Anzahl von Dateien im Cache
#define FILE_CONTENT_SIZE 1024 // Maximale Dateigröße

typedef struct CacheEntry {
    char *filename;
    char *content;
    size_t content_size;
    struct CacheEntry *next;
} CacheEntry;

typedef struct FileCache {
    CacheEntry *head;
    size_t size;
    pthread_mutex_t lock;
} FileCache;

FileCache *init_cache() {
    FileCache *cache = (FileCache *)malloc(sizeof(FileCache));
    if (!cache) {
        perror("Fehler beim Initialisieren des Caches");
        exit(EXIT_FAILURE);
    }
    cache->head = NULL;
    cache->size = 0;
    pthread_mutex_init(&cache->lock, NULL);
    return cache;
}

void free_cache(FileCache *cache) {
    pthread_mutex_lock(&cache->lock);
    CacheEntry *current = cache->head;
    while (current) {
        CacheEntry *to_free = current;
        current = current->next;
        free(to_free->filename);
        free(to_free->content);
        free(to_free);
    }
    pthread_mutex_unlock(&cache->lock);
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}

CacheEntry *find_in_cache(FileCache *cache, const char *filename) {
    CacheEntry *current = cache->head;
    while (current) {
        if (strcmp(current->filename, filename) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void remove_oldest(FileCache *cache) {
    if (!cache->head) return;
    CacheEntry *to_remove = cache->head;
    cache->head = cache->head->next;
    free(to_remove->filename);
    free(to_remove->content);
    free(to_remove);
    cache->size--;
}

void add_to_cache(FileCache *cache, const char *filename, const char *content, size_t content_size) {
    pthread_mutex_lock(&cache->lock);

    if (find_in_cache(cache, filename)) {
        pthread_mutex_unlock(&cache->lock);
        return; // Datei ist bereits im Cache
    }

    while (cache->size >= CACHE_SIZE) {
        remove_oldest(cache);
    }

    CacheEntry *new_entry = (CacheEntry *)malloc(sizeof(CacheEntry));
    if (!new_entry) {
        perror("Fehler beim Hinzufügen zum Cache");
        pthread_mutex_unlock(&cache->lock);
        return;
    }

    new_entry->filename = strdup(filename);
    new_entry->content = (char *)malloc(content_size);
    if (!new_entry->content) {
        perror("Fehler beim Speichern des Datei-Inhalts");
        free(new_entry);
        pthread_mutex_unlock(&cache->lock);
        return;
    }
    memcpy(new_entry->content, content, content_size);
    new_entry->content_size = content_size;
    new_entry->next = cache->head;
    cache->head = new_entry;
    cache->size++;

    pthread_mutex_unlock(&cache->lock);
}

char *get_file(FileCache *cache, const char *filename) {
    pthread_mutex_lock(&cache->lock);
    CacheEntry *entry = find_in_cache(cache, filename);
    if (entry) {
        pthread_mutex_unlock(&cache->lock);
        return entry->content; // Datei gefunden
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Fehler beim Öffnen der Datei");
        pthread_mutex_unlock(&cache->lock);
        return NULL;
    }

    char *content = (char *)malloc(FILE_CONTENT_SIZE);
    if (!content) {
        perror("Fehler beim Zuweisen des Datei-Speichers");
        fclose(file);
        pthread_mutex_unlock(&cache->lock);
        return NULL;
    }

    size_t read_size = fread(content, 1, FILE_CONTENT_SIZE, file);
    fclose(file);

    add_to_cache(cache, filename, content, read_size);
    pthread_mutex_unlock(&cache->lock);
    return content;
}

int main() {
    FileCache *cache = init_cache();

    // Beispiel: Dateien in den Cache laden und abrufen
    char *content1 = get_file(cache, "file1.txt");
    if (content1) {
        printf("Inhalt von file1.txt: %s\n", content1);
    }

    char *content2 = get_file(cache, "file2.txt");
    if (content2) {
        printf("Inhalt von file2.txt: %s\n", content2);
    }

    // Cache freigeben
    free_cache(cache);
    return 0;
}
