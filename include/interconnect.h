/**
 * @file interconnect.h
 * @brief 
 */

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "single_cache.h"
#include "distributed_directory.h"
#include "queue.h"

typedef enum {
    READ_REQUEST,       // Cache to Memory
    READ_ACKNOWLEDGE,   // Memory to Cache
    INVALIDATE,         // Memory to Cache
    INVALIDATE_ACK,     // Cache to Memory
    WRITE_REQUEST,      // Cache to Memory
    WRITE_ACKNOWLEDGE   // Memory to Cache
} message_type;

typedef struct {
    message_type type;
    int sourceId;
    int destId;
    int address;
} message_t;

typedef struct node {
    directory_t *directory;
    cache_t *cache;
} node_t;

typedef struct interconnect {
    Queue* incomingQueue;  // Queue for incoming messages
    Queue* outgoingQueue;  // Queue for outgoing messages
    node_t nodeList[NUM_PROCESSORS];
} interconnect_t;

interconnect_t* createInterconnect();
// add other function declarations here

void freeInterconnect(interconnect_t* interconnect);

#endif // INTERCONNECT_H

