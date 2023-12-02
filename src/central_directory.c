/**
 * @file central_directory.c
 * @brief Implement a central directory based cache coherence protocol. 
 */

/**
 * @brief Initialize the directory
 * 
 * @param numLines 
 * @return directory_t* 
 */
directory_t* initializeDirectory(int numLines) {
    directory_t* dir = (directory_t*)malloc(sizeof(directory_t));
    dir->lines = (directory_entry_t*)malloc(sizeof(directory_entry_t) * numLines);
    dir->numLines = numLines;
    pthread_mutex_init(&dir->lock, NULL);
    
    for (int i = 0; i < numLines; i++) {
        dir->lines[i].state = DIR_UNCACHED;
        for(int j = 0; j < NUM_PROCESSORS; j++){
            dir->lines[i].existsInCache[j] = false;
        }
        dir->lines[i].owner = -1;
        pthread_mutex_init(&dir->lines[i].lock, NULL);
    }
    return dir;
}

/**
 * @brief Helper function to find the directory index for a given address
 * 
 * @param address 
 * @return int 
 */
int directoryIndex(int address) {
    return address % NUM_LINES; // IS THIS CORRECT
}

/**
 * @brief Update the directory entry for a given address
 * 
 * @param directory 
 * @param address 
 * @param processorId 
 * @param newState 
 */
void updateDirectoryEntry(directory_t* directory, int address, int processorId, directory_state newState) {
    int index = directoryIndex(address);
    pthread_mutex_lock(&directory->lines[index].lock);
    directory->lines[index].state = newState;
    directory->lines[index].owner = (newState == DIR_EXCLUSIVE_MODIFIED) ? processorId : -1;
    pthread_mutex_unlock(&directory->lines[index].lock);
}

/**
 * @brief Invalidate the directory entry for a given address
 * 
 * @param directory 
 * @param address 
 */
void invalidateDirectoryEntry(directory_t* directory, int address) {
   int index = directoryIndex(address);
   pthread_mutex_lock(&directory->lines[index].lock);
   directory->lines[index].state = DIR_UNCACHED;
   for(int j = 0; j < NUM_PROCESSORS; j++){
      directory->lines[index].existsInCache[j] = false;
   }
   directory->lines[index].owner = -1;
   pthread_mutex_unlock(&directory->lines[index].lock);
}

/**
 * @brief Add a processor to the directory entry's existsInCache bits
 * 
 * @param directory 
 * @param address 
 * @param processorId 
 */
void addProcessorToEntry(directory_t* directory, int address, int processorId) {
   int index = directoryIndex(address);
   pthread_mutex_lock(&directory->lines[index].lock);
   directory->lines[index].existsInCache[processorId] = true;
   pthread_mutex_unlock(&directory->lines[index].lock);
}

/**
 * @brief Remove a processor from the directory entry's existsInCache bits
 * 
 * @param directory 
 * @param address 
 * @param processorId 
 */
void removeProcessorFromEntry(directory_t* directory, int address, int processorId) {
   int index = directoryIndex(address);
   pthread_mutex_lock(&directory->lines[index].lock);
   directory->lines[index].existsInCache[processorId] = false;
   pthread_mutex_unlock(&directory->lines[index].lock);
}

/**
 * @brief Broadcast an invalidate to all processors except the owner for a given address
 * 
 * @param directory 
 * @param address 
 */
void broadcastInvalidate(directory_t* directory, int address) {
    int index = directoryIndex(address);
    pthread_mutex_lock(&directory->lines[index].lock);
    if(directory->lines[index].state != DIR_UNCACHED) {
        // Send invalidate message to all processors except the owner
        for(int j = 0; j < NUM_PROCESSORS; j++){
            if(j != directory->lines[index].owner){
                // sendMessage(INVALIDATE, address, j);
            }
        }
    }
    pthread_mutex_unlock(&directory->lines[index].lock);
}


// Check if the cache is in a consistent state with the directory
bool checkCacheConsistency(directory_t* directory, int lineIndex, int processorId) {
   // Implement consistency checking logic here
   return true; // placeholder return
}


/**
 * @brief Free the directory
 * 
 * @param dir 
 */
void freeDirectory(directory_t* dir) {
   if(dir != NULL) {
      if(dir->lines != NULL) {
         for(int i = 0; i < dir->numLines; i++) {
            pthread_mutex_destroy(&dir->lines[i].lock);
         }
         free(dir->lines);
      }
      pthread_mutex_destroy(&dir->lock);
      free(dir);
   }
}

/**
 * @brief initializeSystem
 * 
 */
void initializeSystem(void) {
    // Initialize the central directory
    directory = initializeDirectory(NUM_LINES);

    // Initialize the interconnect
    interconnect = createInterconnect();

    // Initialize and connect all caches to the interconnect
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        caches[i] = initializeCache(S, E, B, i);  // Initialize cache for processor 'i'
        connectCacheToInterconnect(caches[i], interconnect);
    }

    // Connect the interconnect to the central directory
    connectInterconnectToDirectory(interconnect, directory);

}

/**
 * @brief cleanup System
 * 
 */
void cleanupSystem(void) {
    // Cleanup caches
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        if (caches[i] != NULL) {
            freeCache(caches[i]);
        }
    }

    // Cleanup interconnect
    if (interconnect != NULL) {
        freeInterconnect(interconnect);
    }

    // Finally, free the central directory
    if (directory != NULL) {
        freeDirectory(directory);
    }
}

/**
 * @brief 
 * 
 * @param request_line 
 */
void executeInstruction(char *request_line) {
   // Implement instruction execution logic...
}

/**
 * @brief Prints information about what parameters the program requires and it's format.
 * 
*/
void displayUsage(void) {
   return;
}

