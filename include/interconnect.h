/**
 * @file interconnect.h
 * @brief 
 */

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include "utils.h"

// Function Declarations
interconnect_t *createInterconnect(int numLines, int s, int e, int b);
message_t createMessage(message_type type, int srcId, int destId, int address);
void processMessageQueue(interconnect_t* interconnect);
void handleReadRequest(interconnect_t* interconnect, message_t msg);
void handleWriteRequest(interconnect_t* interconnect, message_t msg);
void handleInvalidateRequest(interconnect_t* interconnect, message_t msg);
void handleReadAcknowledge(interconnect_t* interconnect, message_t msg);
void handleInvalidateAcknowledge(interconnect_t* interconnect, message_t msg);
void handleWriteAcknowledge(interconnect_t* interconnect, message_t msg);
void freeInterconnect();
void readFromCache(cache_t *cache, int address);
void writeToCache(cache_t *cache, int address);
void fetchFromDirectory(directory_t* directory, int address, int requestingProcessorId);
void sendFetch(interconnect_t* interconnect, int destId, unsigned long address);
void sendReadData(interconnect_t* interconnect, int destId, unsigned long address, bool exclusive);
void freeDirectory(directory_t* dir);
void updateDirectoryState(directory_t* directory, unsigned long address, directory_state newState);
bool updateCacheLineState(cache_t *cache, unsigned long address, block_state newState);

#endif // INTERCONNECT_H

