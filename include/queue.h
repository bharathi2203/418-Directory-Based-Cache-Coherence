/**
 * @file queue.h
 * @brief 
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <pthread.h>

// Queue node
typedef struct QueueNode {
    void* data;
    struct QueueNode* next;
} QueueNode;

// Queue structure
typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int size;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} Queue;

// Function declarations
Queue* createQueue();
void enqueue(Queue* q, void* data);
void* dequeue(Queue* q);
void* peekQueue(const Queue* q);
int queueSize(const Queue* q);
bool isQueueEmpty(const Queue* q);
void freeQueue(Queue* q);

#endif // QUEUE_H
