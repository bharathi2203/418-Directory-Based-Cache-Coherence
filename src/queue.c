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

void printQueue(Queue* q) {
    QueueNode* temp = q->front;
    int i = 0;

    while (temp != NULL) {
        message_t *m = (temp->data);
        printf("\n [#%d] Type: %d, srcID: %d, destId: %d, address: %lu\n", i, m->type, m->sourceId, m->destId, m->address);
        temp = temp->next;
        i += 1;
    }
}


void enqueue(Queue* q, message_type type, int sourceId, int destId, unsigned long address) {
    if (q == NULL) {
        return;
    }

    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (newNode == NULL) {
        return;
    }

    newNode->data = (message_t*)malloc(sizeof(message_t));
    newNode->data->type = type;
    newNode->data->sourceId = sourceId;
    newNode->data->destId = destId;
    newNode->data->address = address;

    newNode->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }

    q->size++;
    
    // printf("\nEQ: type: %d, srcID: %d, destId: %d, address: %lu\n", m->type, m->sourceId, m->destId, m->address);
}

/**
 * @brief 
 * 
 * @param q 
 * @return void* 
 */
message_t* dequeue(Queue* q) {
    if (q == NULL || q->front == NULL) {
        return NULL;
    }

    QueueNode* temp = q->front;
    message_t* data = temp->data;

    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    q->size--;
    
    // printf("\n DQ:type: %d, srcID: %d, destId: %d, address: %lu\n", m->type, m->sourceId, m->destId, m->address);
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
