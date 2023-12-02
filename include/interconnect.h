/**
 * @file interconnect.h
 * @brief 
 */

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include <stdbool.h>
#include <pthread.h>

typedef enum {
    READ_REQUEST,       // Cache to Memory
    READ_ACKNOWLEDGE,   // Memory to Cache
    INVALIDATE,         // Memory to Cache
    INVALIDATE_ACK,     // Cache to Memory
    WRITE_REQUEST,      // Cache to Memory
    WRITE_UPDATE,       // Cache to Memory or Cache to Cache (depending on policy)
    WRITE_ACKNOWLEDGE,  // Memory to Cache
    UPDATE              // Memory to Cache
} message_type;

typedef struct {
    message_type type;  // The type of message being sent
    int sourceId;       // ID of the sending cache or memory
    int destId;         // ID of the destination cache or memory
    int address;        // The memory address involved in the message
} message_t;

typedef struct {
    int processorId;
    cache_t cache;  // Assuming cache_t is another defined struct
} processor_t;

typedef struct {
   message_t *queue;
    // Additional attributes like bandwidth, latency, etc.
} interconnect_t;

// Function declarations for interconnect 
// Initialize the interconnect
interconnect_t *createInterconnect(int num_processors);

// Send a message via the interconnect
void interconnectSendMessage(interconnect_t *interconnect, message_t message);

// Process messages from the interconnect queue
void interconnectProcessMessages(interconnect_t *interconnect);

// Free resources associated with the interconnect
void interconnectFree(interconnect_t *interconnect);

int broadcastMessage(int source, message_t message);

#endif // INTERCONNECT_H
