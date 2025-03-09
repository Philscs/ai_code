#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_PRIORITY 10
#define MAX_SIZE 10

typedef struct {
    uint32_t priority;
    void* message;
} message_t;

typedef struct queue {
    message_t* messages;
    int capacity;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} queue_t;

queue_t* create_queue(int capacity) {
    queue_t* q = malloc(sizeof(queue_t));
    if (!q) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    q->messages = malloc(capacity * sizeof(message_t));
    if (!q->messages) {
        printf("Memory allocation failed\n");
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->size = 0;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);

    return q;
}

void enqueue(queue_t* q, message_t* msg) {
    int priority = msg->priority;

    if (priority < 0 || priority > MAX_PRIORITY) {
        printf("Invalid priority\n");
        return;
    }

    pthread_mutex_lock(&q->mutex);
    while (q->size >= q->capacity - 1) {
        printf("Queue is full\n");
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    q->messages[q->size] = *msg;
    q->size++;
    pthread_mutex_unlock(&q->mutex);

    printf("Message added to queue with priority %u\n", priority);
}

message_t* dequeue(queue_t* q) {
    int priority = -1;

    pthread_mutex_lock(&q->mutex);
    if (is_empty(q)) {
        printf("Queue is empty\n");
        pthread_cond_wait(&q->cond, &q->mutex);
        goto end;
    }
    priority = q->messages[0].priority;
    message_t temp = q->messages[0];
    for (int i = 0; i < q->size - 1; i++) {
        q->messages[i] = q->messages[i + 1];
    }
    q->size--;
    pthread_mutex_unlock(&q->mutex);

end:
    pthread_cond_signal(&q->cond);
    return &temp;
}

bool is_empty(queue_t* q) {
    return q->size == 0;
}

int get_size(queue_t* q) {
    return q->size;
}
