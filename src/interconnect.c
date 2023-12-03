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

   // Initialize the mutex for thread-safe operations
   pthread_mutex_init(&interconnect->mutex, NULL);

   return interconnect;
}

/**
 * @brief 
 * 
 * @param interconnect 
 * @param message 
 */
void interconnectSendMessage(interconnect_t *interconnect, message_t message) {
    // Check if the interconnect and its queue are valid
    if (interconnect == NULL || interconnect->queue == NULL) {
        fprintf(stderr, "Interconnect or queue not initialized.\n");
        return;
    }

    // Lock the interconnect's mutex to ensure thread-safe access to the queue
    pthread_mutex_lock(&interconnect->mutex);

    // Check if the queue is full before attempting to send the message
    if (interconnect->count >= interconnect->capacity) {
        fprintf(stderr, "Interconnect queue is full. Message dropped.\n");
        pthread_mutex_unlock(&interconnect->mutex);
        return;
    }

    // Allocate space for the new message
    message_t *newMessage = (message_t *)malloc(sizeof(message_t));
    if (newMessage == NULL) {
        fprintf(stderr, "Failed to allocate memory for new message.\n");
        pthread_mutex_unlock(&interconnect->mutex);
        return;
    }

    // Copy the message data into the newly allocated space
    *newMessage = message;

    // Insert the message into the queue
    // Assuming there's a function like queueInsert() to add an item to the queue
    queueInsert(interconnect->queue, newMessage);
    interconnect->count++;

    // Unlock the interconnect's mutex after inserting the message
    pthread_mutex_unlock(&interconnect->mutex);

    // Optional: Signal a condition variable to wake up any threads waiting for messages on the interconnect
    // pthread_cond_signal(&interconnect->condition);
}



/**
 * @brief 
 * 
 * @param source 
 * @param message 
 * @param interconnect 
 * @return int 
 */
int broadcastMessage(int source, message_t message, interconnect_t *interconnect) {
   if (!interconnect) return -1;

   for (int i = 0; i < NUM_PROCESSORS; ++i) {
      if (i == source) continue; // Skip the source processor

      message_t newMessage = message;
      newMessage.sourceId = source;
      newMessage.destId = i; // Set the destination processor

      interconnectSendMessage(interconnect, newMessage);
   }

   return 0;
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
      pthread_mutex_destroy(&interconnect->mutex);
      free(interconnect);
   }
}
