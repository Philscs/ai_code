#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_QUEUE_SIZE 100

typedef struct {
    int data[MAX_QUEUE_SIZE];
    int front;
    int rear;
    pthread_mutex_t lock;
} Queue;

void initQueue(Queue* queue) {
    queue->front = -1;
    queue->rear = -1;
    pthread_mutex_init(&queue->lock, NULL);
}

int isQueueEmpty(Queue* queue) {
    return (queue->front == -1 && queue->rear == -1);
}

int isQueueFull(Queue* queue) {
    return (queue->rear + 1) % MAX_QUEUE_SIZE == queue->front;
}

void enqueue(Queue* queue, int item) {
    pthread_mutex_lock(&queue->lock);

    if (isQueueFull(queue)) {
        printf("Queue is full. Cannot enqueue item.\n");
        pthread_mutex_unlock(&queue->lock);
        return;
    }

    if (isQueueEmpty(queue)) {
        queue->front = 0;
        queue->rear = 0;
    } else {
        queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    }

    queue->data[queue->rear] = item;

    pthread_mutex_unlock(&queue->lock);
}

int dequeue(Queue* queue) {
    pthread_mutex_lock(&queue->lock);

    if (isQueueEmpty(queue)) {
        printf("Queue is empty. Cannot dequeue item.\n");
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }

    int item = queue->data[queue->front];

    if (queue->front == queue->rear) {
        queue->front = -1;
        queue->rear = -1;
    } else {
        queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    }

    pthread_mutex_unlock(&queue->lock);

    return item;
}

int main() {
    Queue queue;
    initQueue(&queue);

    enqueue(&queue, 10);
    enqueue(&queue, 20);
    enqueue(&queue, 30);

    printf("Dequeued item: %d\n", dequeue(&queue));
    printf("Dequeued item: %d\n", dequeue(&queue));
    printf("Dequeued item: %d\n", dequeue(&queue));

    return 0;
}
