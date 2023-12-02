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
   if (interconnect == NULL || interconnect->queue == NULL) return;

   pthread_mutex_lock(&interconnect->mutex);

   // Copy the message into dynamically allocated memory
   message_t *messageCopy = (message_t *)malloc(sizeof(message_t));
   if (!messageCopy) {
      pthread_mutex_unlock(&interconnect->mutex);
      return; // Memory allocation failure
   }
   *messageCopy = message;

   // Enqueue the message copy
   enqueue(interconnect->queue, messageCopy);

   pthread_mutex_unlock(&interconnect->mutex);
}

/**
 * @brief 
 * 
 * @param arg 
 * @return void* 
 */
void *interconnectProcessMessages(void *arg) {
   interconnect_t *interconnect = (interconnect_t *)arg;
   if (interconnect == NULL || interconnect->queue == NULL) return;

   while (true) { // Replace with a condition to stop the thread
      pthread_mutex_lock(&interconnect->mutex);

      if (isQueueEmpty(interconnect->queue)) {
         pthread_mutex_unlock(&interconnect->mutex);
         continue; // Consider adding a wait condition here
      }

      message_t *message = (message_t *)dequeue(interconnect->queue);
      pthread_mutex_unlock(&interconnect->mutex);

      if (message) {
            // Process the message
            // e.g., if (message->type == READ_REQUEST) { ... }

         free(message); // Free the dequeued message
      }
   }

   return NULL;
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
