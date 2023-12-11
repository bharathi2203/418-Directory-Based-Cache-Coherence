#include "queue.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
/**
 * @brief Create a Queue object
 * 
 * @return Queue* 
 */
Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    printf("createQueue %p\n", (void*)q);
    if (q) {
        q->front = q->rear = NULL;
        q->size = 0;
    }
    return q;
}

/**
 * @brief 
 * 
 * @param q 
 * @param data 
 */

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



void enqueue(Queue* q, void* data) {
    if (q == NULL) {
        return;
    }

    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (newNode == NULL) {
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
    message_t* m = (message_t*)(data);
    printf("\nEQ: type: %d, srcID: %d, destId: %d, address: %lu\n", m->type, m->sourceId, m->destId, m->address);
}

/**
 * @brief 
 * 
 * @param q 
 * @return void* 
 */
void* dequeue(Queue* q) {
    if (q == NULL || q->front == NULL) {
        return NULL;
    }

    QueueNode* temp = q->front;
    void* data = temp->data;

    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    q->size--;
    message_t* m = (message_t*)(data);
    printf("\n DQ:type: %d, srcID: %d, destId: %d, address: %lu\n", m->type, m->sourceId, m->destId, m->address);
    return data;
}

/**
 * @brief 
 * 
 * @param q 
 * @return void* 
 */
void* peekQueue(const Queue* q) {
    if (q == NULL || q->front == NULL) {
        return NULL;
    }
    return q->front->data;
}

/**
 * @brief 
 * 
 * @param q 
 * @return int 
 */
int queueSize(const Queue* q) {
    if (q == NULL) {
        return -1;
    }
    return q->size;
}

/**
 * @brief 
 * 
 * @param q 
 * @return true 
 * @return false 
 */
bool isQueueEmpty(const Queue* q) {
    return (q == NULL || q->size == 0);
}

/**
 * @brief 
 * 
 * @param q 
 */
void freeQueue(Queue* q) {
    if (q == NULL) {
        return;
    }

    while (!isQueueEmpty(q)) {
        dequeue(q);
    }

    free(q);
}
