/**
 * @file interconnect.c
 * @brief 
 */
#include <interconnect.h>

/**
 * @brief Create and Initializes the interconnect system with specified parameters.
 * 
 * @return interconnect_t* A pointer to the initialized interconnect structure.
 */
interconnect_t *createInterconnect() {
   interconnect_t *interconnect = (interconnect_t *)malloc(sizeof(interconnect_t));
   if (interconnect == NULL) {
      return NULL;
   }

   // Initialize the message queue
   interconnect->queue = createQueue();
   if (!interconnect->queue) {
      free(interconnect);
      return NULL;
   }

   return interconnect;
}

// Function to connect cache to interconnect
/**
 * @brief 
 * 
 * @param cache 
 * @param interconnect 
 */
void connectCacheToInterconnect(cache_t* cache, interconnect_t* interconnect) {
   if (cache != NULL) {
      cache->interconnect = interconnect;
   }
}

// Function to connect interconnect to directory
/**
 * @brief 
 * 
 * @param interconnect 
 * @param directory 
 */
void connectInterconnectToDirectory(interconnect_t* interconnect, directory_t* directory) {
   if (directory != NULL) {
      directory->interconnect = interconnect;
   }
}


// Helper function to create a message
static message_t createMessage(message_type type, int srcId, int destId, int address) {
    message_t msg;
    msg.type = type;
    msg.sourceId = srcId;
    msg.destId = destId;
    msg.address = address;
    return msg;
}

void sendReadRequest(interconnect_t* interconnect, int srcId, int destId, int address) {
    message_t msg = createMessage(READ_REQUEST, srcId, destId, address);
    enqueue(interconnect->queue, msg);
}

void sendExclusiveReadRequest(interconnect_t* interconnect, int srcId, int destId, int address) {
    message_t msg = createMessage(READ_ACKNOWLEDGE, srcId, destId, address);
    enqueue(interconnect->queue, msg);
}

void sendReadData(interconnect_t* interconnect, int srcId, int destId, int address) {
    message_t msg = createMessage(WRITE_REQUEST, srcId, destId, address);
    enqueue(interconnect->queue, msg);
}

void sendWriteData(interconnect_t* interconnect, int srcId, int destId, int address) {
    message_t msg = createMessage(WRITE_UPDATE, srcId, destId, address);
    enqueue(interconnect->queue, msg);
}

void sendInvalidateRequest(interconnect_t* interconnect, int srcId, int destId, int address) {
    message_t msg = createMessage(INVALIDATE, srcId, destId, address);
    enqueue(interconnect->queue, msg);
}

void sendInvalidateAck(interconnect_t* interconnect, int srcId, int destId, int address) {
    message_t msg = createMessage(INVALIDATE_ACK, srcId, destId, address);
    enqueue(interconnect->queue, msg);
}

void processMessageQueue(interconnect_t* interconnect) {
    while (!isQueueEmpty(interconnect->queue)) {
        message_t msg = dequeue(interconnect->queue);
        // Here you would process the message. 
        // For example, you could call a function in the cache or directory
        // to handle the message based on its type.
        switch (msg.type) {
            case READ_REQUEST:
                // Process read request
                break;
            case READ_ACKNOWLEDGE:
                // Process read acknowledge
                break;
            // ... and so on for other message types
        }
    }
}


/**
 * @brief 
 * 
 * @param interconnect 
 */
void freeInterconnect(interconnect_t *interconnect) {
   if (interconnect != NULL) {
      if (interconnect->queue != NULL) {
         freeQueue(interconnect->queue);
      }
      free(interconnect);
   }
}
