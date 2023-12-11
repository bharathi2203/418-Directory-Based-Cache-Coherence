/**
 * @file interconnect.h
 * @brief 
 */

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "utils.h"

// Function Declarations
interconnect_t *createInterconnect(int numLines, int s, int e, int b);
message_t createMessage(message_type type, int srcId, int destId, unsigned long address);
void processMessageQueue();
void handleReadRequest(message_t msg);
void handleWriteRequest(message_t msg);
void handleInvalidateRequest(message_t msg);
void handleReadAcknowledge(message_t msg);
void handleInvalidateAcknowledge(message_t msg);
void handleWriteAcknowledge(message_t msg);
void freeInterconnect();
void readFromCache(cache_t *cache, unsigned long address, int srcID);
void writeToCache(cache_t *cache, unsigned long address, int srcID);
void fetchFromDirectory(directory_t* directory, unsigned long address, int requestingProcessorId);
void sendFetch(int srcId, int destId, unsigned long address);
void sendReadData(int srcId, int destId, unsigned long address, bool exclusive);
void freeDirectory(directory_t* dir);
void updateDirectoryState(directory_t* directory, unsigned long address, directory_state newState);
bool updateCacheLineState(cache_t *cache, unsigned long address, block_state newState);

#endif // INTERCONNECT_H

