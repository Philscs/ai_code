#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

// Node struct for the queue
typedef struct Node {
    void *data;
    struct Node *next;
} Node;

// Queue struct
typedef struct Queue {
    Node *front;
    Node *rear;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    bool deadlock_detected;
} Queue;

// Initialize the queue
Queue *queue_init() {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    q->deadlock_detected = false;
    return q;
}

// Enqueue an element into the queue
void enqueue(Queue *q, void *data) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->data = data;
    new_node->next = NULL;

    pthread_mutex_lock(&q->lock);

    if (q->rear == NULL) {
        q->front = q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

// Dequeue an element from the queue
void *dequeue(Queue *q) {
    pthread_mutex_lock(&q->lock);

    while (q->front == NULL) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    Node *temp = q->front;
    void *data = temp->data;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    pthread_mutex_unlock(&q->lock);
    return data;
}

// Deadlock detection and recovery simulation
void detect_and_recover_deadlock(Queue *q) {
    // Simulate deadlock detection using a timeout mechanism
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5; // 5-second timeout

    pthread_mutex_lock(&q->lock);

    if (!pthread_cond_timedwait(&q->not_empty, &q->lock, &ts)) {
        // No deadlock detected
        pthread_mutex_unlock(&q->lock);
        return;
    }

    // Deadlock detected
    fprintf(stderr, "Deadlock detected! Initiating recovery...\n");
    q->deadlock_detected = true;

    // Recovery logic: Clear the queue
    Node *current = q->front;
    while (current != NULL) {
        Node *next = current->next;
        free(current);
        current = next;
    }
    q->front = q->rear = NULL;
    q->deadlock_detected = false;

    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

// Destroy the queue
void queue_destroy(Queue *q) {
    pthread_mutex_lock(&q->lock);

    Node *current = q->front;
    while (current != NULL) {
        Node *next = current->next;
        free(current);
        current = next;
    }

    pthread_mutex_unlock(&q->lock);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    free(q);
}

// Example usage
int main() {
    Queue *q = queue_init();

    // Enqueue some data
    enqueue(q, "Task 1");
    enqueue(q, "Task 2");

    // Dequeue data
    printf("Dequeued: %s\n", (char *)dequeue(q));
    printf("Dequeued: %s\n", (char *)dequeue(q));

    // Simulate deadlock detection and recovery
    detect_and_recover_deadlock(q);

    queue_destroy(q);
    return 0;
}
