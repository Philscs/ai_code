#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CACHE_SIZE 100

typedef struct {
    char* filename;
    char* data;
    size_t size;
    time_t lastModified;
} CacheEntry;

typedef struct {
    CacheEntry* entries[MAX_CACHE_SIZE];
    size_t count;
    pthread_mutex_t lock;
} FileCache;

FileCache* createCache() {
    FileCache* cache = (FileCache*)malloc(sizeof(FileCache));
    cache->count = 0;
    pthread_mutex_init(&cache->lock, NULL);
    return cache;
}

void destroyCache(FileCache* cache) {
    for (size_t i = 0; i < cache->count; i++) {
        free(cache->entries[i]->filename);
        free(cache->entries[i]->data);
        free(cache->entries[i]);
    }
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}

CacheEntry* findEntry(FileCache* cache, const char* filename) {
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->entries[i]->filename, filename) == 0) {
            return cache->entries[i];
        }
    }
    return NULL;
}

void removeEntry(FileCache* cache, CacheEntry* entry) {
    for (size_t i = 0; i < cache->count; i++) {
        if (cache->entries[i] == entry) {
            free(entry->filename);
            free(entry->data);
            free(entry);
            cache->entries[i] = cache->entries[cache->count - 1];
            cache->count--;
            break;
        }
    }
}

void addToCache(FileCache* cache, const char* filename, const char* data, size_t size, time_t lastModified) {
    pthread_mutex_lock(&cache->lock);

    CacheEntry* entry = findEntry(cache, filename);
    if (entry != NULL) {
        removeEntry(cache, entry);
    }

    if (cache->count == MAX_CACHE_SIZE) {
        removeEntry(cache, cache->entries[0]);
    }

    entry = (CacheEntry*)malloc(sizeof(CacheEntry));
    entry->filename = strdup(filename);
    entry->data = (char*)malloc(size);
    memcpy(entry->data, data, size);
    entry->size = size;
    entry->lastModified = lastModified;

    cache->entries[cache->count] = entry;
    cache->count++;

    pthread_mutex_unlock(&cache->lock);
}

char* getFromCache(FileCache* cache, const char* filename, size_t* size, time_t* lastModified) {
    pthread_mutex_lock(&cache->lock);

    CacheEntry* entry = findEntry(cache, filename);
    if (entry == NULL) {
        pthread_mutex_unlock(&cache->lock);
        return NULL;
    }

    *size = entry->size;
    *lastModified = entry->lastModified;

    char* data = (char*)malloc(entry->size);
    memcpy(data, entry->data, entry->size);

    pthread_mutex_unlock(&cache->lock);

    return data;
}

void invalidateCache(FileCache* cache, const char* filename) {
    pthread_mutex_lock(&cache->lock);

    CacheEntry* entry = findEntry(cache, filename);
    if (entry != NULL) {
        removeEntry(cache, entry);
    }

    pthread_mutex_unlock(&cache->lock);
}

int main() {
    FileCache* cache = createCache();

    // Usage example
    const char* filename = "example.txt";
    const char* data = "This is some example data.";
    size_t size = strlen(data);
    time_t lastModified = time(NULL);

    addToCache(cache, filename, data, size, lastModified);

    size_t cachedSize;
    time_t cachedLastModified;
    char* cachedData = getFromCache(cache, filename, &cachedSize, &cachedLastModified);
    if (cachedData != NULL) {
        printf("Cached data: %s\n", cachedData);
        free(cachedData);
    } else {
        printf("Data not found in cache.\n");
    }

    destroyCache(cache);

    return 0;
}
