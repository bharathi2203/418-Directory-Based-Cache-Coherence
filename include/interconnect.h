/**
 * @file interconnect.h
 * @brief 
 */

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

typedef enum {
    READ_REQUEST, READ_ACKNOWLEDGE, INVALIDATE, INVALIDATE_ACK,
    WRITE_REQUEST, WRITE_UPDATE, WRITE_ACKNOWLEDGE, UPDATE
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
void interconnectSendMessage(interconnect_t* interconnect, message_t message);
void broadcastMessage(int source, message_t message, interconnect_t* interconnect);
void connectCacheToInterconnect(cache_t* cache, interconnect_t* interconnect);
void connectInterconnectToDirectory(interconnect_t* interconnect, directory_t* directory);
void freeInterconnect(interconnect_t* interconnect);

#endif // INTERCONNECT_H

