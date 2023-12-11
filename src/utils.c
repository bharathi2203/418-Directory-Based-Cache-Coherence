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
 * @brief Free directory_t object. 
 * 
 * @param dir 
 */
void freeDirectory(directory_t* dir) {
    if (dir) {
        if (dir->lines) {
            free(dir->lines);
        }
        free(dir);
    }
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
 * @brief Reads data from a specified address in the cache. 
 *
 * This function attempts to read data from the cache at a given address. 
 * It first checks if the desired line is in the cache (cache hit). If 
 * found and valid, it increments the hit counter and updates the line's 
 * usage for the LRU policy. 
 * 
 * In case of a cache miss, it increments the miss counter and then 
 * fetches the required line from the directory or another cache, 
 * updating the local cache with the shared state of the line. This 
 * function handles both hit and miss scenarios, ensuring proper cache 
 * coherence and consistency.
 * 
 * @param cache         The cache from which data is to be read.
 * @param address       The memory address to read from.
 */
void readFromCache(cache_t *cache, int address) {
    int index = directoryIndex(address);
    set_t *set = &cache->setList[index];
    line_t *line = findLineInSet(set, address);

    if (line != NULL && line->valid && line->tag == address) {
        // Cache hit
        printf("Cache HIT: Processor %d reading address %d\n", cache->processor_id, address);
        cache->hitCount++;
        updateLineUsage(line);  // Update line usage on hit
    } else {
        // Cache miss
        printf("Cache MISS: Processor %d reading address %d\n", cache->processor_id, address);
        cache->missCount++;
        fetchFromDirectory(interconnect->nodeList[cache->processor_id].directory, address, cache->processor_id);
        addLineToCacheSet(cache, set, address, SHARED);
    }
}

/**
 * @brief Writes data to a specified address in the cache.
 * 
 * This function is responsible for writing data to the cache at a given address. 
 * It checks if the target line is present and valid in the cache (cache hit). If 
 * a hit occurs, it updates the line's state to MODIFIED, marks it as dirty, and 
 * updates its usage for LRU policy. 
 * 
 * In the event of a cache miss, it increments the miss counter, fetches the line 
 * from the directory or another cache, and adds it to the cache set with a 
 * MODIFIED state. This function also notifies the directory of any changes, 
 * ensuring the line is updated to the exclusive modified state in the directory, 
 * maintaining cache coherence.
 * 
 * @param cache             The cache where data is to be written.
 * @param address           The memory address to write to.
 */
void writeToCache(cache_t *cache, int address) {
    int index = directoryIndex(address);
    set_t *set = &cache->setList[index];
    line_t *line = findLineInSet(set, address);

    if (line != NULL && line->valid && calculateTag(address, main_S, main_B) == address) {
        // Cache hit
        printf("Cache HIT: Processor %d writing to address %d\n", cache->processor_id, address);
        cache->hitCount++;
        line->state = MODIFIED;
        line->isDirty = true;
        updateLineUsage(line);  // Update line usage on hit
    } else {
        // Cache miss
        printf("Cache MISS: Processor %d writing to address %d\n", cache->processor_id, address);
        cache->missCount++;
        fetchFromDirectory(interconnect->nodeList[cache->processor_id].directory, address, cache->processor_id);
        addLineToCacheSet(cache, set, address, MODIFIED);
    }
    // Notify the directory that this cache now has a modified version of the line
    updateDirectory(interconnect->nodeList[cache->processor_id].directory, address, cache->processor_id, DIR_EXCLUSIVE_MODIFIED);
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