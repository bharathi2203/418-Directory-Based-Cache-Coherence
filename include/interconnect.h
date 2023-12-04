/**
 * @file interconnect.h
 * @brief 
 */

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

typedef enum {
    READ_REQUEST, READ_ACKNOWLEDGE, INVALIDATE, INVALIDATE_ACK,
    WRITE_REQUEST, WRITE_UPDATE, WRITE_ACKNOWLEDGE, FETCH
} message_type;

typedef struct {
    message_type type;
    int sourceId;
    int destId;
    int address;
} message_t;

typedef struct interconnect {
    Queue* queue;
} interconnect_t;

interconnect_t* createInterconnect();

void sendReadRequest(interconnect_t* interconnect, int srcId, int destId, int address);
void sendExclusiveReadRequest(interconnect_t* interconnect, int srcId, int destId, int address);
void sendReadData(interconnect_t* interconnect, int srcId, int destId, int address);
void sendWriteData(interconnect_t* interconnect, int srcId, int destId, int address);
void sendInvalidateRequest(interconnect_t* interconnect, int srcId, int destId, int address);
void sendInvalidateAck(interconnect_t* interconnect, int srcId, int destId, int address);
void processMessageQueue(interconnect_t* interconnect);

void freeInterconnect(interconnect_t* interconnect);

#endif // INTERCONNECT_H

