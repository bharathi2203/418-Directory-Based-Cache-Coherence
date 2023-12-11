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

#endif // INTERCONNECT_H

