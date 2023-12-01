/**
 * @file interconnect.h
 * @brief 
 */

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include <stdbool.h>
#include <pthread.h>

// Interconnect struct 
typedef struct {
   int temp;
} Interconnect;


// Function declarations for interconnect 
Interconnect* createInterconnect(int num_processors);
int sendData(int source, int dest, void* data);
int receiveData(int source, int dest, void* data);
int broadcastMessage(int source, void* data);
void processInterconnectQueue(void);
void freeInterconnect(Interconnect* interconnect);

#endif // INTERCONNECT_H
