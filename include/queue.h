/**
 * @file queue.h
 * @brief 
 */
#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h> 


typedef enum {
    READ_REQUEST,       // Cache to Memory
    READ_ACKNOWLEDGE,   // Memory to Cache
    INVALIDATE,         // Memory to Cache
    INVALIDATE_ACK,     // Cache to Memory
    STATE_UPDATE,       // Cache to Cache 
    WRITE_REQUEST,      // Cache to Memory
    WRITE_ACKNOWLEDGE,  // Memory to Cache
    FETCH               // unsure if it need it, maybe removed it earlier?
} message_type;

typedef struct {
    message_type type;
    int sourceId;
    int destId;
    unsigned long address;
} message_t;

typedef struct QueueNode {
    message_t* data;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int size;
} Queue;

Queue* createQueue();
void enqueue(Queue* q, message_type type, int sourceId, int destId, unsigned long address);
message_t* dequeue(Queue* q);
void* peekQueue(const Queue* q);
int queueSize(const Queue* q);
bool isQueueEmpty(const Queue* q);
void freeQueue(Queue* q);
void printQueue(Queue* q);

#endif // QUEUE_H
