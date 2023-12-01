/**
 * @file interconnect.c
 * @brief 
 */
#include <interconnect.h>


// Function declarations for interconnect 
Interconnect* createInterconnect(int num_processors);
/*
createInterconnect:
   - Purpose: Initializes the interconnect system with specified parameters.
   - Parameters: Number of cores, interconnect topology, bandwidth, latency characteristics, etc.
   - Returns: A pointer to the initialized interconnect structure.
*/

int sendData(int source, int dest, void* data);
/*
sendData:
   - Purpose: Handles sending data from one cache/core to another.
   - Parameters: Source and destination identifiers, data to be sent.
   - Returns: Status of the data transfer operation.
*/

int receiveData(int source, int dest, void* data);
/*
receiveData:
   - Purpose: Manages the reception of data at a cache/core.
   - Parameters: Receiver identifier, buffer for received data.
   - Returns: Amount of data received and status.
*/

int broadcastMessage(int source, void* data);
/*
broadcastMessage:
   - Purpose: Sends a message to all cores/caches in the system.
   - Parameters: Message to be broadcasted.
   - Returns: Status of the broadcast operation.
*/

void processInterconnectQueue(void);
/*
processInterconnectQueue:
   - Purpose: Processes queued messages or data transfers.
   - Parameters: None, or time/frame reference for simulation.
   - Returns: None. Updates internal state.
*/


void freeInterconnect(Interconnect* interconnect);
/*
freeInterconnect:
   - Purpose: Frees resources associated with the interconnect.
   - Parameters: Pointer to the interconnect.
   - Returns: None.
*/
