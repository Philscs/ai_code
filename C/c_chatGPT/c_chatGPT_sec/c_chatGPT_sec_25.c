#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 256

typedef struct {
    int priority;
    char message[MAX_MESSAGE_LENGTH];
} Message;

typedef struct {
    Message *queue[MAX_MESSAGES];
    int size;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} MessageQueue;

MessageQueue* create_queue() {
    MessageQueue* mq = (MessageQueue*)malloc(sizeof(MessageQueue));
    if (!mq) {
        perror("Failed to allocate memory for message queue");
        exit(EXIT_FAILURE);
    }

    mq->size = 0;
    pthread_mutex_init(&mq->lock, NULL);
    pthread_cond_init(&mq->not_empty, NULL);
    pthread_cond_init(&mq->not_full, NULL);

    return mq;
}

void destroy_queue(MessageQueue* mq) {
    pthread_mutex_destroy(&mq->lock);
    pthread_cond_destroy(&mq->not_empty);
    pthread_cond_destroy(&mq->not_full);
    free(mq);
}

void enqueue(MessageQueue* mq, const char* message, int priority) {
    pthread_mutex_lock(&mq->lock);

    while (mq->size == MAX_MESSAGES) {
        pthread_cond_wait(&mq->not_full, &mq->lock);
    }

    Message* msg = (Message*)malloc(sizeof(Message));
    if (!msg) {
        perror("Failed to allocate memory for message");
        pthread_mutex_unlock(&mq->lock);
        exit(EXIT_FAILURE);
    }

    msg->priority = priority;
    strncpy(msg->message, message, MAX_MESSAGE_LENGTH - 1);
    msg->message[MAX_MESSAGE_LENGTH - 1] = '\0';

    int i;
    for (i = mq->size - 1; i >= 0 && mq->queue[i]->priority < priority; i--) {
        mq->queue[i + 1] = mq->queue[i];
    }
    mq->queue[i + 1] = msg;
    mq->size++;

    pthread_cond_signal(&mq->not_empty);
    pthread_mutex_unlock(&mq->lock);
}

Message* dequeue(MessageQueue* mq) {
    pthread_mutex_lock(&mq->lock);

    while (mq->size == 0) {
        pthread_cond_wait(&mq->not_empty, &mq->lock);
    }

    Message* msg = mq->queue[0];
    for (int i = 1; i < mq->size; i++) {
        mq->queue[i - 1] = mq->queue[i];
    }
    mq->size--;

    pthread_cond_signal(&mq->not_full);
    pthread_mutex_unlock(&mq->lock);

    return msg;
}

void process_message(Message* msg) {
    if (msg) {
        printf("Processed message with priority %d: %s\n", msg->priority, msg->message);
        free(msg);
    }
}

int main() {
    MessageQueue* mq = create_queue();

    pthread_t producer, consumer;

    void* producer_thread(void* arg);
    for (int i = 0; i < 10; i++) {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "Message %d", i);
        enqueue(mq, buffer, rand() % 10);
    }
    

    void* consumer_thread(void* arg);
    for (int i = 0; i < 10; i++) {
        Message* msg = dequeue(mq);
        process_message(msg);
    }

    pthread_create(&producer, NULL, producer_thread, NULL);
    pthread_create(&consumer, NULL, consumer_thread, NULL);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    destroy_queue(mq);
    return 0;
}
