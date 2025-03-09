#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_QUEUE_SIZE 100

typedef struct {
    int priority;
    char* message;
} Message;

typedef struct {
    Message queue[MAX_QUEUE_SIZE];
    int front;
    int rear;
    pthread_mutex_t mutex;
    pthread_cond_t notEmpty;
} MessageQueue;

void initMessageQueue(MessageQueue* mq) {
    mq->front = 0;
    mq->rear = 0;
    pthread_mutex_init(&mq->mutex, NULL);
    pthread_cond_init(&mq->notEmpty, NULL);
}

void destroyMessageQueue(MessageQueue* mq) {
    pthread_mutex_destroy(&mq->mutex);
    pthread_cond_destroy(&mq->notEmpty);
}

void enqueue(MessageQueue* mq, int priority, const char* message) {
    pthread_mutex_lock(&mq->mutex);

    while ((mq->rear + 1) % MAX_QUEUE_SIZE == mq->front) {
        pthread_cond_wait(&mq->notEmpty, &mq->mutex);
    }

    mq->queue[mq->rear].priority = priority;
    mq->queue[mq->rear].message = strdup(message);
    mq->rear = (mq->rear + 1) % MAX_QUEUE_SIZE;

    pthread_mutex_unlock(&mq->mutex);
}

Message dequeue(MessageQueue* mq) {
    pthread_mutex_lock(&mq->mutex);

    while (mq->front == mq->rear) {
        pthread_cond_wait(&mq->notEmpty, &mq->mutex);
    }

    Message message = mq->queue[mq->front];
    mq->front = (mq->front + 1) % MAX_QUEUE_SIZE;

    pthread_mutex_unlock(&mq->mutex);

    return message;
}

void* workerThread(void* arg) {
    MessageQueue* mq = (MessageQueue*)arg;

    while (1) {
        Message message = dequeue(mq);
        // Process the message
        printf("Processing message: %s\n", message.message);
        free(message.message);
    }

    return NULL;
}

int main() {
    MessageQueue mq;
    initMessageQueue(&mq);

    // Create worker threads
    pthread_t worker1, worker2;
    pthread_create(&worker1, NULL, workerThread, &mq);
    pthread_create(&worker2, NULL, workerThread, &mq);

    // Enqueue messages
    enqueue(&mq, 2, "High priority message");
    enqueue(&mq, 1, "Medium priority message");
    enqueue(&mq, 3, "Low priority message");

    // Wait for worker threads to finish
    pthread_join(worker1, NULL);
    pthread_join(worker2, NULL);

    destroyMessageQueue(&mq);

    return 0;
}
