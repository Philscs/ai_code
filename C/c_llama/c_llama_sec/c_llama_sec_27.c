#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_CACHE_SIZE 100 // Maximum cache size in MB

// Structure to represent a file entry in the cache
typedef struct FileEntry {
    char filename[256];
    bool cached;
} FileEntry;

// Structure to represent the cache
typedef struct Cache {
    pthread_mutex_t mutex; // Mutex for thread safety
    FileEntry* entries; // Array of file entries
    size_t num_entries; // Number of cache entries
    size_t capacity; // Current cache capacity
} Cache;

// Function to initialize a new cache instance
Cache* create_cache() {
    Cache* cache = malloc(sizeof(Cache));
    pthread_mutex_init(&cache->mutex, NULL);
    cache->entries = malloc(MAX_CACHE_SIZE * sizeof(FileEntry));
    cache->num_entries = 0;
    cache->capacity = MAX_CACHE_SIZE;
    return cache;
}

// Function to add a file entry to the cache
void add_file_to_cache(Cache* cache, const char* filename) {
    // Check if file already exists in cache
    for (size_t i = 0; i < cache->num_entries; i++) {
        if (strcmp(cache->entries[i].filename, filename) == 0) {
            printf("File %s already exists in the cache.\n", filename);
            return;
        }
    }

    // Add file to cache
    for (size_t i = 0; i < cache->num_entries; i++) {
        if (!cache->entries[i].cached) {
            cache->entries[i].filename[0] = '\0';
            break;
        }
    }

    memcpy(cache->entries[cache->num_entries].filename, filename, strlen(filename));
    cache->entries[cache->num_entries].cached = true;
    cache->num_entries++;
}

// Function to remove a file entry from the cache
void remove_file_from_cache(Cache* cache, const char* filename) {
    // Find index of file in cache
    size_t idx = -1;
    for (size_t i = 0; i < cache->num_entries; i++) {
        if (strcmp(cache->entries[i].filename, filename) == 0) {
            idx = i;
            break;
        }
    }

    // Check if file exists in cache
    if (idx != -1) {
        for (size_t i = 0; i < cache->num_entries; i++) {
            if (!cache->entries[i].cached) {
                cache->entries[i].filename[0] = '\0';
                break;
            }
        }

        // Remove file from cache
        memcpy(cache->entries[idx].filename, "", 1);
        cache->entries[idx].cached = false;
        cache->num_entries--;
    } else {
        printf("File %s does not exist in the cache.\n", filename);
    }
}

// Function to check if a file exists in the cache
bool check_file_in_cache(Cache* cache, const char* filename) {
    // Check if file exists in cache
    for (size_t i = 0; i < cache->num_entries; i++) {
        if (strcmp(cache->entries[i].filename, filename) == 0) {
            return true;
        }
    }

    return false;
}

// Function to update the file time stamp when a new version of the file is written
void invalidate_file_in_cache(Cache* cache, const char* filename) {
    // Check if file exists in cache
    for (size_t i = 0; i < cache->num_entries; i++) {
        if (strcmp(cache->entries[i].filename, filename) == 0) {
            cache->entries[i].cached = false;
            break;
        }
    }

    // File not in cache
}

int main() {
    Cache* cache = create_cache();
    pthread_mutex_lock(&cache->mutex);
    add_file_to_cache(cache, "test.txt");
    printf("File exists: %d\n", check_file_in_cache(cache, "test.txt"));
    remove_file_from_cache(cache, "test.txt");
    printf("File exists: %d\n", check_file_in_cache(cache, "test.txt"));

    pthread_mutex_unlock(&cache->mutex);
    pthread_destroy.cache;

    return 0;
}
