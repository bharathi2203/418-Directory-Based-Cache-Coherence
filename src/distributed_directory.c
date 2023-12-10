/**
 * @file distributed_directory.c
 * @brief Implement a central directory based cache coherence protocol. 
 */

#include "single_cache.h"
#include "distributed_directory.h"
#include "interconnect.h"

/**
 * @brief Global Structs
 * 
 */
interconnect_t *interconnect = NULL;
csim_stats_t *system_stats = NULL;

/**
 * @brief Initialize the directory
 * 
 * @param numLines 
 * @param interconnect 
 * @return directory_t* 
 */
directory_t* initializeDirectory(int numLines, interconnect_t* interconnect) {
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
    dir->interconnect = interconnect; // Link the directory to the interconnect

    // Initialize each line in the directory
    for (int i = 0; i < numLines; i++) {
        dir->lines[i].state = DIR_UNCACHED;
        memset(dir->lines[i].existsInCache, false, sizeof(bool) * NUM_PROCESSORS);
        dir->lines[i].owner = -1;
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
   return address % NUM_LINES;
}

/**
 * @brief Get the Current Time object
 * 
 * @return unsigned long 
 */
unsigned long getCurrentTime() {
    return (unsigned long)time(NULL);
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
 * @brief initializeSystem
 * 
 */
void initializeSystem(void) {
    // Allocate and initialize interconnect with queues
    interconnect = createInterconnect();
    interconnect->incomingQueue = createQueue();
    interconnect->outgoingQueue = createQueue();

    // Initialize NUMA nodes, each with its own cache and directory
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        interconnect->nodeList[i].directory = initializeDirectory(NUM_LINES, interconnect);
        interconnect->nodeList[i].cache = initializeCache(s, e, b, i); // 's', 'e', and 'b' need to be defined based on the system's configuration
    }
}


/**
 * @brief cleanup System
 * 
 */
void cleanupSystem(void) {
    // Cleanup all NUMA nodes
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        if (interconnect->nodeList[i].cache) {
            freeCache(interconnect->nodeList[i].cache);
        }
        if (interconnect->nodeList[i].directory) {
            freeDirectory(interconnect->nodeList[i].directory);
        }
    }

    // Cleanup interconnect queues
    if (interconnect) {
        if (interconnect->incomingQueue) {
            freeQueue(interconnect->incomingQueue);
        }
        if (interconnect->outgoingQueue) {
            freeQueue(interconnect->outgoingQueue);
        }
        free(interconnect);
    }
}

/**
 * @brief Fetches a cache line from the directory for a requesting processor. 
 *
 * This function handles fetching a cache line from the directory based on its 
 * current state and updates its state accordingly. If the cache line is exclusively 
 * modified, it fetches the line from the owning cache and invalidates it there. 
 *
 * For uncached or shared lines, it fetches the line directly from the main memory. 
 * Finally, it updates the directory entry, setting the line to shared state 
 * and recording its presence in the requesting processor's cache.
 * 
 * @param directory                 The directory from which the cache line is fetched.
 * @param address                   The memory address of the cache line to fetch.
 * @param requestingProcessorId     The ID of the processor requesting the cache line.
 */
void fetchFromDirectory(directory_t* directory, int address, int requestingProcessorId) {
    int index = directoryIndex(address);
    directory_entry_t* line = &directory->lines[index];

    // Check the state of the directory line
    if (line->state == DIR_EXCLUSIVE_MODIFIED) {
        // If the line is exclusively modified, we need to fetch it from the owning cache
        int owner = line->owner;
        if (owner != -1) {
            // Invalidate the line in the owner's cache
            sendInvalidate(directory->interconnect, owner, address);

            // Simulate transferring the line to the requesting cache
            sendFetch(directory->interconnect, requestingProcessorId, address);
        }
    } else if (line->state == DIR_UNCACHED || line->state == DIR_SHARED) {
        // If the line is uncached or shared, fetch it from the memory
        sendReadData(directory->interconnect, requestingProcessorId, address, (line->state == DIR_UNCACHED));
    }

    // Update the directory entry
    line->state = DIR_SHARED;
    line->owner = -1;
    for (int i = 0; i < NUM_PROCESSORS; i++) {
        line->existsInCache[i] = false;
    }
    line->existsInCache[requestingProcessorId] = true;
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
        updateLineUsage(set, line);  // Update line usage on hit
    } else {
        // Cache miss
        printf("Cache MISS: Processor %d reading address %d\n", cache->processor_id, address);
        cache->missCount++;
        fetchFromDirectory(cache->interconnect->nodeList[cache->processor_id].directory, address, cache->processor_id);
        addLineToCacheSet(set, address, SHARED);
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

    if (line != NULL && line->valid && line’s tag == address) {
        // Cache hit
        printf("Cache HIT: Processor %d writing to address %d\n", cache->processor_id, address);
        cache->hitCount++;
        line->state = MODIFIED;
        line->isDirty = true;
        updateLineUsage(set, line);  // Update line usage on hit
    } else {
        // Cache miss
        printf("Cache MISS: Processor %d writing to address %d\n", cache->processor_id, address);
        cache->missCount++;
        fetchFromDirectory(cache’s interconnect->nodeList[cache->processor_id].directory, address, cache->processor_id);
        addLineToCacheSet(set, address, MODIFIED);
    }
    // Notify the directory that this cache now has a modified version of the line
    updateDirectory(cache->interconnect->nodeList[cache->processor_id].directory, address, cache->processor_id, DIR_EXCLUSIVE_MODIFIED);
}


/**
 * @brief This function finds a line in the cache set
 * 
 * @param set 
 * @param address 
 * @return line_t* 
 */
line_t *findLineInSet(set_t *set, unsigned long address) {
    for (int i = 0; i < set->associativity; i++) {
        if (set->lines[i].tag == address && set->lines[i].valid) {
            return &set->lines[i];
        }
    }
    return NULL; // Line not found in cache set
}


/**
 * @brief Adds a line to the cache set using the LRU replacement policy
 * 
 * @param set 
 * @param address 
 * @param state 
 */
void addLineToCacheSet(set_t *set, unsigned long address, block_state state) {
    unsigned long tag = calculateTag(address, set->S, set->B);
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
void updateLineUsage(set_t *set, line_t *line) {
    line->lastUsed = getCurrentTime(); 
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
                sendInvalidate(directory->interconnect, i, address);
            }
        }
    }
}

/**
 * @brief Sends a read data acknowledgment message via the interconnect system. 
 * 
 * It creates a READ_ACKNOWLEDGE message specifying the source ID (often the directory's 
 * ID), the destination cache ID, and the memory address of the data. The message is 
 * then enqueued in the interconnect's outgoing queue, signaling that the requested data 
 * is ready to be sent to the requesting cache node.
 * 
 * @param interconnect 
 * @param destId 
 * @param address 
 * @param exclusive 
 */
void sendReadData(interconnect_t* interconnect, int destId, unsigned long address, bool exclusive) {
    // Create a READ_ACKNOWLEDGE message
    message_t readDataMsg = {
        .type = READ_ACKNOWLEDGE,
        .sourceId = -1, // -1 or a specific ID if the directory has an ID
        .destId = destId,
        .address = address
    };

    // Enqueue the message to the outgoing queue
    enqueue(interconnect->outgoingQueue, readDataMsg);
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
    enqueue(interconnect->outgoingQueue, invalidateMsg);
}

/**
 * @brief Facilitates fetching data from a cache node in response to a cache miss in another node. 
 * 
 * On encountering a cache miss, this function is used to create and send a FETCH message 
 * through the interconnect system to the cache node that currently holds the required data. 
 * This ensures efficient data retrieval and consistency across the distributed cache system.
 * 
 * @param interconnect 
 * @param destId 
 * @param address 
 */
void sendFetch(interconnect_t* interconnect, int destId, unsigned long address) {
    // Create a FETCH message
    message_t fetchMsg = {
        .type = FETCH,
        .sourceId = -1, // -1 or a specific ID if the directory has an ID
        .destId = destId,
        .address = address
    };

    // Enqueue the message to the outgoing queue
    enqueue(interconnect->outgoingQueue, fetchMsg);
}


/**
 * @brief Parse a line from the trace file and perform the corresponding operation
 * 
 * @param line 
 * @param interconnect 
 */
void processTraceLine(char *line, interconnect_t* interconnect) {
    int processorId;
    char operation;
    int address;

    // Parse the line to extract the processor ID, operation, and address
    if (sscanf(line, "%d %c %x", &processorId, &operation, &address) == 3) {
        // Convert the address to the local node's index, assuming the address includes the node information
        int localNodeIndex = addressToLocalNodeIndex(address);
        switch (operation) {
            case 'R':
                // Send a read request to the interconnect queue
                enqueue(interconnect->incomingQueue, createMessage(READ_REQUEST, processorId, localNodeIndex, address));
                break;
            case 'W':
                // Send a write request to the interconnect queue
                enqueue(interconnect->incomingQueue, createMessage(WRITE_REQUEST, processorId, localNodeIndex, address));
                break;
            default:
                fprintf(stderr, "Unknown operation '%c' in trace file\n", operation);
                break;
        }
    } else {
        fprintf(stderr, "Invalid line format in trace file: %s\n", line);
    }
}


/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tracefile>\n", argv[0]);
        return 1;
    }

    // Initialize the system
    initializeSystem();

    // Open the trace file
    FILE *traceFile = fopen(argv[1], "r");
    if (traceFile == NULL) {
        perror("Error opening trace file");
        return 1;
    }

    // Process each line in the trace file
    char line[1024];
    while (fgets(line, sizeof(line), traceFile)) {
        // Here you we call parse the line and service the instruction
        
    }

    // Cleanup and close the file
    fclose(traceFile);

    // Cleanup resources
    cleanupSystem();

    return 0;
}