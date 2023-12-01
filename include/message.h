/**
 * @file message.h
 * @brief 
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdbool.h>
#include <pthread.h>

// Message Structure
typedef struct {
       int messageType; // Types like READ, WRITE, INVALIDATE, etc.
       int sourceId;    // ID of the sending processor or cache
       int targetId;    // ID of the target processor or cache
       int address;     // Memory address involved
       int data;        // Data to be sent (for write messages)
} Message;


// Processor Structure:
typedef struct {
       int processorId;
       Cache cache; // Assuming Cache is another defined struct
       // Other processor-specific attributes
} Processor;

// Communication Interface:
typedef struct {
       MessageQueue messages; // Queue of messages (FIFO)
       // Other attributes like bandwidth, latency, etc.
} CommunicationInterface;

// Function declarations for interconnect 
Message* createMessage(int messageType, int src, int dest, int addr, int data);
void freeMessage(Message* message);

#endif // MESSAGE_H


