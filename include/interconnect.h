/**
 * @file interconnect.h
 * @brief 
 */

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "single_cache.h"
#include "distributed_directory.h"
#include "queue.h"


/*
The `message_type` enum defines different types of messages used for communication between caches and memory in the cache coherence system:

1. **READ_REQUEST** (Cache to Memory): 
        Used when a cache requests data from memory.
2. **READ_ACKNOWLEDGE** (Memory to Cache): 
        Sent from memory to cache to acknowledge a READ_REQUEST.
3. **INVALIDATE** (Memory to Cache): 
        Instructs a cache to invalidate a specific cache line.
4. **INVALIDATE_ACK** (Cache to Memory): 
        Cache's response to an INVALIDATE message, acknowledging that the line has been invalidated.
5. **STATE_UPDATE** (Cache to Cache): 
        Communicates state changes of cache lines between caches.
6. **WRITE_REQUEST** (Cache to Memory): 
        Issued when a cache needs to write data to memory.
7. **WRITE_ACKNOWLEDGE** (Memory to Cache): 
        Memory's response to a WRITE_REQUEST, confirming the write operation.

*/
typedef enum {
    READ_REQUEST,       // Cache to Memory
    READ_ACKNOWLEDGE,   // Memory to Cache
    INVALIDATE,         // Memory to Cache
    INVALIDATE_ACK,     // Cache to Memory
    STATE_UPDATE,       // Cache to Cache 
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

// Function Declarations
interconnect_t *createInterconnect(int numLines, int s, int e, int b);
static message_t createMessage(message_type type, int srcId, int destId, int address);
void processMessageQueue(interconnect_t* interconnect);
void handleReadRequest(interconnect_t* interconnect, message_t msg);
void handleWriteRequest(interconnect_t* interconnect, message_t msg);
void handleInvalidateRequest(interconnect_t* interconnect, message_t msg);
void handleReadAcknowledge(interconnect_t* interconnect, message_t msg);
void handleInvalidateAcknowledge(interconnect_t* interconnect, message_t msg);
void handleWriteAcknowledge(interconnect_t* interconnect, message_t msg);
void freeInterconnect(interconnect_t *interconnect);

#endif // INTERCONNECT_H

