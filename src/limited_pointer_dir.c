/**
 * @file limited_pointer_directory.c
 * @brief Implement a limited pointer scheme based cache coherence protocol. 
 */

#include <limited_pointer_dir.h> 
/**
 * @brief Initialize the directory
 * 
 * @param numLines 
 * @return lp_directory_t* 
 */
lp_directory_t* initializeDirectory(int numLines) {
    lp_directory_t* dir = (lp_directory_t*)malloc(sizeof(lp_directory_t));
    dir->lines = (lp_directory_entry_t*)malloc(sizeof(lp_directory_entry_t) * numLines);
    dir->numLines = numLines;
    pthread_mutex_init(&dir->lock, NULL);
    
    for (int i = 0; i < numLines; i++) {
        dir->lines[i].numSharedBy = 0;
        dir->lines[i].state = DIR_UNCACHED;
        for(int j = 0; j < NUM_POINTERS; j++){
            dir->lines[i].nodes[j] = -1;
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
 // Are you assuming a NUMA machine? The central directory source has a single directory for all caches so we can just index into the directory using the address 
 // if NUMA machines, the mmenry is split across processors (slide deck #12) so we should find the home node for this address and then search for the entry there. 
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
void updateDirectoryEntry(lp_directory_t* directory, int address, int processorId, directory_state newState) {
    int index = directoryIndex(address);
    lp_directory_entry_t* entry = directory->lines[index];
    pthread_mutex_lock(entry->lock);
    entry->state = newState;
    entry->owner = (newState == DIR_EXCLUSIVE_MODIFIED) ? processorId : -1;
    entry->nodes[numSharedBy] = processorId;
    entry->numSharedBy++;
    pthread_mutex_unlock(entry->lock);
}

/**
 * @brief Invalidate the directory entry for a given address
 * 
 * @param directory 
 * @param address 
 */
void invalidateDirectoryEntry(lp_directory_t* directory, int address) {
   int index = directoryIndex(address);
   lp_directory_entry_t* entry = directory->lines[index];
   pthread_mutex_lock(entry->lock);
   entry->numSharedBy = 0;
   entry->state = DIR_UNCACHED;

   for(int j = 0; j < NUM_POINTERS; j++){
      entry->nodes[j] = -1;
   }
   entry->owner = -1;
   pthread_mutex_unlock(entry->lock);
}

// TODO: error check, if overflow we need to kick old sharer out or http://15418.courses.cs.cmu.edu/spring2013/article/25? 
/**
 * @brief Add a processor to the directory entry's existsInCache bits
 * 
 * @param directory 
 * @param address 
 * @param processorId 
 */
void addProcessorToEntry(lp_directory_t* directory, int address, int processorId) {
   int index = directoryIndex(address);
   lp_directory_entry_t* entry = directory->lines[index];
   pthread_mutex_lock(entry->lock);
   entry->nodes[numSharedBy] = processorId;
   entry->numSharedBy++;
   pthread_mutex_unlock(entry->lock);
}

/**
 * @brief Remove a processor from the directory entry's existsInCache bits
 * 
 * @param directory 
 * @param address 
 * @param processorId 
 */
void removeProcessorFromEntry(lp_directory_t* directory, int address, int processorId) {
   int index = directoryIndex(address);
   lp_directory_entry_t* entry = directory->lines[index];
   pthread_mutex_lock(entry->lock);
   for(int i = 0; i < entry->numSharedBy; i++) {
    if(entry->nodes[i] == processorId) {
        entry->nodes[i] = -1; 
    }
   }
   // Do we have to update the state here or does updateDirectoryEntry handle that? 
   pthread_mutex_unlock(entry->lock);
}

/**
 * @brief Broadcast an invalidate to all processors except the owner for a given address
 * 
 * @param directory 
 * @param address 
 */
void broadcastInvalidate(lp_directory_t* directory, int address) {
    int index = directoryIndex(address);
    lp_directory_entry_t* entry = directory->lines[index];
    pthread_mutex_lock(entry->lock);
    if(entry->state != DIR_UNCACHED) {
        // Send invalidate message to all processors except the owner
        for(int j = 0; j < numSharedBy; j++){
            if(entry->node[j] != entry->owner){
                // sendMessage(INVALIDATE, address, j);
            }
        }
    }
    pthread_mutex_unlock(entry->lock);
}


// Check if the cache is in a consistent state with the directory
bool checkCacheConsistency(lp_directory_t* directory, int lineIndex, int processorId) {
   // Implement consistency checking logic here
   return true; // placeholder return
}


/**
 * @brief Free the directory
 * 
 * @param dir 
 */
void freeDirectory(lp_directory_t* dir) {
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
    lp_directory_t* directory = initializeDirectory(NUM_LINES);

    // Initialize the interconnect
    interconnect_t* interconnect = createInterconnect();

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

