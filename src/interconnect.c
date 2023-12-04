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
