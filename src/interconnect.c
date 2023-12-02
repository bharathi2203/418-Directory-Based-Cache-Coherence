/**
 * @file interconnect.c
 * @brief 
 */
#include <interconnect.h>


/*
createInterconnect:
   - Purpose: Initializes the interconnect system with specified parameters.
   - Parameters: Number of cores, interconnect topology, bandwidth, latency characteristics, etc.
   - Returns: A pointer to the initialized interconnect structure.
*/
interconnect_t *createInterconnect() {
   interconnect_t *interconnect = (interconnect_t *)malloc(sizeof(interconnect_t));
   if (interconnect == NULL) {
      return NULL;
   }

   // Initialize the queue
   interconnect->queue = createQueue();
   if (!interconnect->queue) {
       // Handle queue creation failure
       free(interconnect);
       return NULL;
   }

   // Set up mutex and condition variable
   pthread_mutex_init(&(interconnect->queue->lock), NULL);
   pthread_cond_init(&(interconnect->queue->cond), NULL);

   // Capacity, numMessages, head, and tail are now managed by the Queue struct
   return interconnect;
}


/**
 * @brief 
 * 
 * @param q 
 * @param message 
 */
void interconnectSendMessage(Queue* q, message_t* message) {
   enqueue(q, (void*)message);
}


/**
 * @brief 
 * 
 * @param arg 
 * @return void* 
 */
void* interconnectProcessMessages(interconnect_t *interconnect) {
    Queue* q = interconnect->queue;
    while (true) {  // This condition could be a flag indicating if the simulation is running
        message_t* message = (message_t*)dequeue(q);
        if (message == NULL) {
            // Handle the case where the queue is empty or the simulation is ending
            continue;
        }

        // Process the message based on its type
        // For example:
        if (message->type == READ_REQUEST) {
            // Call a function to handle the read request
        } else if (message->type == INVALIDATE) {
            // Call a function to handle the invalidate message
        }
        // ...

        // free the message after processing
        free(message);

        // Add condition or wait mechanism to break loop and end the thread
    }
    return NULL;
}


void interconnectFree(interconnect_t *interconnect) {
    if (interconnect) {
        free(interconnect->queue);
        free(interconnect);
    }
}