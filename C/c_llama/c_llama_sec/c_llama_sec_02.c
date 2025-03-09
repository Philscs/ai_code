#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// Strukt für die Elemente in der Queue
typedef struct {
    void* data;
    int priority;
} QueueElement;

// Strukt für die Queue
typedef struct {
    sem_t mutex;
    sem_t cond;
    pthread_mutex_t lock;
} Queue;

// Funktion zur Erstellung einer neuen Queue
Queue* queue_create() {
    Queue* q = malloc(sizeof(Queue));
    sem_init(&q->mutex, 0, 1);
    sem_init(&q->cond, 0, 0);
    pthread_mutex_init(&q->lock, NULL);
    return q;
}

// Funktion zum Erstellen eines neuen Elements in der Queue
QueueElement* queue_element_create(void* data, int priority) {
    QueueElement* elem = malloc(sizeof(QueueElement));
    elem->data = data;
    elem->priority = priority;
    return elem;
}

// Funktion zum Hinzufügen eines Elements in die Queue
void queue_push(Queue* q, void* data, int priority) {
    pthread_mutex_lock(&q->lock);
    sem_wait(&q->mutex);
    while (sem_timedwait(&q->cond, NULL, 1000000));
    QueueElement* elem = queue_element_create(data, priority);
    if (elem == NULL) {
        printf("Memory allocation failed\n");
        pthread_mutex_unlock(&q->lock);
        return;
    }
    queue_add(q, elem);
    sem_post(&q->cond);
    sem_post(&q->mutex);
    pthread_mutex_unlock(&q->lock);
}

// Funktion zum Hinzufügen eines Elements in die Queue
void queue_add(Queue* q, QueueElement* elem) {
    // Prioritätsvermeidung
    if (elem->priority > 1 && (q->elements == NULL || ((QueueElement*)q->elements)->priority >= 
elem->priority)) {
        printf("Prioritätsverletzung\n");
        return;
    }
    elem->next = q->elements;
    q->elements = elem;
}

// Funktion zum Entleeren der Queue
void queue_pop(Queue* q) {
    pthread_mutex_lock(&q->lock);
    sem_wait(&q->mutex);
    while (q->elements == NULL || q->cond.wait(q->cond, 1000000));
    QueueElement* elem = q->elements;
    q->elements = elem->next;
    if (elem != NULL) {
        free(elem->data);
        free(elem);
    }
    sem_post(&q->mutex);
    pthread_mutex_unlock(&q->lock);
}

// Funktion zum Erhalten des Front-Elements der Queue
void* queue_peek(Queue* q) {
    pthread_mutex_lock(&q->lock);
    sem_wait(&q->mutex);
    while (q->elements == NULL || q->cond.wait(q->cond, 1000000));
    void* data = ((QueueElement*)q->elements)->data;
    pthread_mutex_unlock(&q->lock);
    return data;
}

// Funktion zum Beenden der Queue
void queue_destroy(Queue* q) {
    pthread_mutex_lock(&q->lock);
    sem_wait(&q->mutex);
    while (q->elements != NULL || q->cond.wait(q->cond, 1000000));
    if (q->elements != NULL) {
        free(((QueueElement*)q->elements)->data);
        free(((QueueElement*)q->elements));
    }
    pthread_mutex_unlock(&q->lock);
    sem_destroy(&q->mutex);
    sem_destroy(&q->cond);
    pthread_mutex_destroy(&q->lock);
}

int main() {
    Queue* q = queue_create();
    void* data1 = malloc(sizeof(int));
    *data1 = 10;
    int priority1 = 1;

    void* data2 = malloc(sizeof(char));
    *data2 = 'A';
    int priority2 = 0;

    queue_push(q, data1, priority1);
    queue_push(q, data2, priority2);

    printf("Front Element:\n");
    printf("%d\n", *(int*)queue_peek(q));

    queue_pop(q);

    printf("\n");

    return 0;
}
