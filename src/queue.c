#include "queue.h"
#include <stdlib.h>

// Create a new queue
Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (q) {
        q->front = q->rear = NULL;
        q->size = 0;
        pthread_mutex_init(&q->lock, NULL);
        pthread_cond_init(&q->cond, NULL);
    }
    return q;
}

// Add an element to the queue
void enqueue(Queue* q, void* data) {
    pthread_mutex_lock(&q->lock);

    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (newNode == NULL) {
        pthread_mutex_unlock(&q->lock);
        return;
    }

    newNode->data = data;
    newNode->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }

    q->size++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

// Remove and return the front element from the queue
void* dequeue(Queue* q) {
    pthread_mutex_lock(&q->lock);
    while (q->size == 0) {
        pthread_cond_wait(&q->cond, &q->lock);
    }

    QueueNode* temp = q->front;
    void* data = temp->data;

    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;

    free(temp);
    q->size--;

    pthread_mutex_unlock(&q->lock);
    return data;
}

// Peek the front element of the queue without removing it
void* peekQueue(const Queue* q) {
    if (isQueueEmpty(q)) return NULL;
    return q->front->data;
}

// Get the size of the queue
int queueSize(const Queue* q) {
    if (q == NULL) return 0;
    return q->size;
}

// Check if the queue is empty
bool isQueueEmpty(const Queue* q) {
    return (q == NULL || q->size == 0);
}

// Free the queue
void freeQueue(Queue* q) {
    pthread_mutex_lock(&q->lock);
    while (!isQueueEmpty(q)) {
        dequeue(q);
    }
    pthread_mutex_unlock(&q->lock);

    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
    free(q);
}
