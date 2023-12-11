#include <utils.h> 

/**
 * @brief Get the Current Time object
 * 
 * @return unsigned long 
 */
unsigned long getCurrentTime() {
    return (unsigned long)time(NULL);
}

/**
 * @brief Adds a line to the cache set using the LRU replacement policy
 * 
 * @param set 
 * @param address 
 * @param state 
 */
void addLineToCacheSet(cache_t *cache, set_t *set, unsigned long address, block_state state) {
    unsigned long tag = address >> (main_S + main_B);
    line_t *oldestLine = NULL;
    unsigned long oldestTime = ULONG_MAX;

    // Look for an empty line or the least recently used line
    for (unsigned int i = 0; i < set->associativity; ++i) {
        line_t *line = &set->lines[i];

        if (!line->valid || line->lastUsed < oldestTime) {
            oldestLine = line;
            oldestTime = line->lastUsed;
        }
    }

    if (oldestLine == NULL) {
        return;
    }

    // Evict the oldest line if necessary
    if (oldestLine->valid && oldestLine->isDirty) {
        // Write back to memory if dirty
        cache->evictionCount++;
        if (oldestLine->isDirty) {
            cache->dirtyEvictionCount++;
        }
    }

    // Update the line with new data
    oldestLine->tag = tag;
    oldestLine->valid = true;
    oldestLine->isDirty = (state == MODIFIED);
    oldestLine->state = state;
    oldestLine->lastUsed = getCurrentTime(); 
}


/**
 * @brief Function to update the last used time of a line during cache access
 * 
 * @param set 
 * @param line 
 */
void updateLineUsage(line_t *line) {
    line->lastUsed = getCurrentTime(); 
}

/**
 * @brief Helper function to find the directory index for a given address
 * 
 * @param address 
 * @return int 
 */
int directoryIndex(int address) {
   return address % NUM_LINES;
}



/**
 * @brief Handles the invalidation of a specific cache line across the caches in the NUMA system. 
 * 
 * It creates an INVALIDATE message to notify a cache node that a particular memory address in its 
 * cache needs to be invalidated. This function plays a pivotal role in maintaining cache coherence 
 * by ensuring that outdated or incorrect data is not used by any of the caches.
 * 
 * @param interconnect 
 * @param destId 
 * @param address 
 */
void sendInvalidate(interconnect_t* interconnect, int destId, unsigned long address) {
    // Create an INVALIDATE message
    message_t invalidateMsg = {
        .type = INVALIDATE,
        .sourceId = -1, // -1 or a specific ID if the directory has an ID
        .destId = destId,
        .address = address
    };

    // Enqueue the message to the outgoing queue
    enqueue(interconnect->outgoingQueue, &invalidateMsg);
}

/**
 * @brief Determines the tag portion of a given memory address for cache storage.
 * 
 * @param address 
 * @param S 
 * @param B 
 * @return unsigned long 
 */
unsigned long calculateTag(unsigned long address, unsigned long S, unsigned long B) {
    return address >> (S + B);
}

/**
 * @brief Computes the set index for a given memory address in the cache.
 * 
 * @param address 
 * @param S 
 * @param B 
 * @return unsigned long 
 */
unsigned long calculateSetIndex(unsigned long address, unsigned long S, unsigned long B) {
    return (address >> B) & ((1UL << S) - 1);
}

/**
 * @brief Initialize the directory
 * 
 * @param numLines 
 * @param interconnect 
 * @return directory_t* 
 */
directory_t* initializeDirectory(int numLines) {
    directory_t* dir = (directory_t*)malloc(sizeof(directory_t));
    if (!dir) {
        return NULL;
    }

    dir->lines = (directory_entry_t*)calloc(numLines, sizeof(directory_entry_t));
    if (!dir->lines) {
        free(dir);
        return NULL;
    }

    dir->numLines = numLines;

    // Initialize each line in the directory
    for (int i = 0; i < numLines; i++) {
        dir->lines[i].state = DIR_UNCACHED;
        memset(dir->lines[i].existsInCache, false, sizeof(bool) * NUM_PROCESSORS);
        dir->lines[i].owner = -1;
    }
    return dir;
}

/**
 * @brief Initialize the cache with the given parameters.
 * 
 * @param s                 Number of set index bits
 * @param e                 Associativity: number of lines per set
 * @param b                 Number of block offset bits
 * @param processor_id      Identifier for the processor to which this cache belongs
 * @return cache_t*         Pointer to the newly allocated cache
 */
cache_t *initializeCache(unsigned int s, unsigned int e, unsigned int b, int processor_id) {
    // Calculate the number of sets from the number of set index bits
    unsigned long numSets = 1UL << s;

    // Allocate memory for the cache structure
    cache_t *cache = malloc(sizeof(cache_t));
    if (cache == NULL) {
        return NULL;
    }

    // Initialize the cache structure
    cache->processor_id = processor_id;
    cache->S = s;
    cache->E = e;
    cache->B = b;
    cache->hitCount = 0;
    cache->missCount = 0;
    cache->evictionCount = 0;
    cache->dirtyEvictionCount = 0;

    // Allocate memory for the array of sets
    cache->setList = malloc(numSets * sizeof(set_t));
    if (cache->setList == NULL) {
        free(cache); // Free the cache structure if set allocation fails
        return NULL;
    }

    // Initialize each set in the cache
    for (unsigned int i = 0; i < numSets; i++) {
        // Allocate memory for the lines within a set
        cache->setList[i].lines = malloc(e * sizeof(line_t));
        cache->setList[i].associativity = e;

        if (cache->setList[i].lines == NULL) {
            // Free previously allocated memory if any line allocation fails
            for (unsigned int j = 0; j < i; j++) {
                free(cache->setList[j].lines);
            }
            free(cache->setList);
            free(cache);
            return NULL;
        }

        // Initialize each line within the set
        for (unsigned int j = 0; j < e; j++) {
            cache->setList[i].lines[j].tag = 0;
            cache->setList[i].lines[j].valid = false;
            cache->setList[i].lines[j].isDirty = false;
            cache->setList[i].lines[j].state = INVALID;
            cache->setList[i].lines[j].lastUsed = 0;
        }
    }

    return cache;
}



/**
 * @brief Releases all memory allocated to a cache.
 * 
 * @param cache The cache to be freed.
 */
void freeCache(cache_t *cache) {
    if (cache == NULL) {
        return;
    }

    // Free each set and its constituent lines
    for (unsigned long i = 0; i < (1UL << cache->S); i++) {
        set_t *set = &cache->setList[i];
        if (set->lines != NULL) {
            free(set->lines); // Free the array of lines in each set
            set->lines = NULL;
        }
    }

    // Free the array of sets
    free(cache->setList);
    cache->setList = NULL;

    // Finally, free the cache itself
    free(cache);
}



/**
 * @brief This function updates the directory after a cache line write
 * 
 * @param directory 
 * @param address 
 * @param cache_id 
 * @param newState 
 */
void updateDirectory(directory_t* directory, unsigned long address, int cache_id, directory_state newState) {
    int index = directoryIndex(address);
    directory_entry_t* line = &directory->lines[index];

    // Update the directory entry based on the new state
    line->state = newState;
    line->owner = (newState == DIR_EXCLUSIVE_MODIFIED) ? cache_id : -1;

    // Invalidate other caches if necessary
    if (newState == DIR_EXCLUSIVE_MODIFIED) {
        for (int i = 0; i < NUM_PROCESSORS; i++) {
            if (i != cache_id) {
                line->existsInCache[i] = false;
                sendInvalidate(interconnect, i, address);
            }
        }
    }
}


/**
 * @brief Marks a specific cache line as invalid.
 * 
 * @param cache 
 * @param address 
 */
void invalidateCacheLine(cache_t *cache, unsigned long address) {
    unsigned long setIndex = (address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = address >> (cache->B + cache->S);

    for (unsigned int i = 0; i < cache->E; ++i) {
        line_t *line = &cache->setList[setIndex].lines[i];
        if (line->tag == tag && line->valid) {
            // Simply invalidate the line without handling dirty data
            line->valid = false;
            line->state = INVALID;
        }
    }
}

/**
 * @brief Locates a specific cache line within a set by its tag.
 * 
 * @param set 
 * @param tag 
 * @return line_t* 
 */
line_t* findLineInSet(set_t set, unsigned long tag) {
    for (unsigned int i = 0; i < set.associativity; ++i) {
        if (set.lines[i].tag == tag && set.lines[i].valid) {
            return &set.lines[i];
        }
    }
    return NULL;  // Line not found
}
