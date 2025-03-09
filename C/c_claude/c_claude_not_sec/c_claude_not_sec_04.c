#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

struct Node {
    void* data;
    size_t size;
    struct Node* next;
    bool in_use;
};

struct MemoryPool {
    struct Node* head;
    pthread_mutex_t lock;
    size_t total_allocated;
};

struct MemoryPool pool = {NULL, PTHREAD_MUTEX_INITIALIZER, 0};

void* custom_malloc(size_t size) {
    pthread_mutex_lock(&pool.lock);

    // Durchsuche den Pool nach einem freien Block
    struct Node* current = pool.head;
    while (current) {
        if (!current->in_use && current->size >= size) {
            current->in_use = true;
            pthread_mutex_unlock(&pool.lock);
            return current->data;
        }
        current = current->next;
    }

    // Wenn kein freier Block gefunden wurde, neuen allokieren
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    if (!new_node) {
        pthread_mutex_unlock(&pool.lock);
        return NULL;
    }
    new_node->data = malloc(size);
    if (!new_node->data) {
        free(new_node);
        pthread_mutex_unlock(&pool.lock);
        return NULL;
    }
    new_node->size = size;
    new_node->in_use = true;
    new_node->next = pool.head;
    pool.head = new_node;

    pool.total_allocated += size;
    pthread_mutex_unlock(&pool.lock);
    return new_node->data;
}

void custom_free(void* ptr) {
    if (!ptr) return;

    pthread_mutex_lock(&pool.lock);

    struct Node* current = pool.head;
    while (current) {
        if (current->data == ptr) {
            current->in_use = false;
            break;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&pool.lock);
}

void garbage_collect() {
    pthread_mutex_lock(&pool.lock);

    struct Node* current = pool.head;
    struct Node* prev = NULL;

    while (current) {
        if (!current->in_use) {
            if (prev) {
                prev->next = current->next;
            } else {
                pool.head = current->next;
            }

            pool.total_allocated -= current->size;
            free(current->data);
            struct Node* to_free = current;
            current = current->next;
            free(to_free);
        } else {
            prev = current;
            current = current->next;
        }
    }

    pthread_mutex_unlock(&pool.lock);
}

void destroy_pool() {
    pthread_mutex_lock(&pool.lock);

    struct Node* current = pool.head;
    while (current) {
        struct Node* to_free = current;
        free(current->data);
        current = current->next;
        free(to_free);
    }
    pool.head = NULL;
    pool.total_allocated = 0;

    pthread_mutex_unlock(&pool.lock);
}

// Beispielmain zur Demonstration
int main() {
    void* block1 = custom_malloc(128);
    void* block2 = custom_malloc(256);
    
    printf("Memory allocated: %zu bytes\n", pool.total_allocated);

    custom_free(block1);
    garbage_collect();

    printf("Memory allocated after GC: %zu bytes\n", pool.total_allocated);

    destroy_pool();
    return 0;
}
