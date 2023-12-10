/**
 * @file single_cache.c
 * @brief cache 
 * 
 * @author BHARATHI SRIDHAR <bsridha2@andrew.cmu.edu>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include "single_cache.h"

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

/**
 * @brief Identifies the cache containing a specific line based on the memory address.
 * 
 * @param address 
 * @return int 
 */
int findCacheWithLine(unsigned long address) {
    int nodeId = address / NUM_PROCESSORS; // Hypothetical way to map address to node ID.
    directory_entry_t* directoryLine = &directory->lines[nodeId];

    // If the line is in EXCLUSIVE or MODIFIED state, that means only one cache has it.
    if (directoryLine->state == DIR_EXCLUSIVE_MODIFIED) {
        return directoryLine->owner;
    }

    // If the line is in SHARED state, we could return any cache that has it,
    // or -1 to indicate it can be fetched from memory.
    if (directoryLine->state == DIR_SHARED) {
        for (int i = 0; i < NUM_PROCESSORS; i++) {
            if (directoryLine->existsInCache[i]) {
                return i; // Return the ID of the first cache found with the line.
            }
        }
    }

    // If the line is UNCACHED or no cache has the line, return -1.
    return -1;
}




/**
 * @brief Handles read requests by checking cache hit/miss and updating state as needed.
 * 
 * @param cache 
 * @param sourceId 
 * @param address 
 */
void processReadRequest(cache_t *cache, int sourceId, unsigned long address) {
    unsigned long setIndex = calculateSetIndex(address, cache->S, cache->B);
    unsigned long tag = calculateTag(address, cache->S, cache->B);
    line_t *line = findLineInSet(cache->setList[setIndex], tag);

    if (line && line->valid) {
        if (line->state == EXCLUSIVE) {
            line->state = SHARED;
            // Notify other caches about the state change
            for (int i = 0; i < NUM_PROCESSORS; i++) {
                // Skip sending notification to the cache where the read request originated
                if (i == cache->processor_id) continue;

                // Create a message to notify other caches about the state change
                message_t stateChangeMsg;
                stateChangeMsg.type = INVALIDATE; // Using INVALIDATE type to signify a state change
                stateChangeMsg.sourceId = cacheId; // ID of the cache initiating the state change
                stateChangeMsg.destId = i; // ID of the cache being notified
                stateChangeMsg.address = address; // Address of the cache line

                // Enqueue the message to the outgoing queue
                enqueue(interconnect->outgoingQueue, stateChangeMsg);
            }
        }
        // Respond with data
        sendReadResponse(interconnect, sourceId, address, line->data);
        // Update line usage for LRU
        updateLineUsage(&cache->setList[setIndex], line);
    } else {
        // Cache miss, fetch from directory or another cache
        fetchDataFromDirectoryOrCache(cache, address);
    }
}


/**
 * @brief Manages write requests, updating cache lines and invalidating others as necessary.
 * 
 * @param cache 
 * @param sourceId 
 * @param address 
 */
void processWriteRequest(cache_t *cache, int sourceId, unsigned long address) {
    unsigned long setIndex = (address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = address >> (cache->B + cache->S);
    line_t *line = findLineInSet(cache->setList[setIndex], tag);

    if (line && line->valid) {
        // Transition any state to MODIFIED
        line->state = MODIFIED;
        line->isDirty = true;
        // Invalidate other caches
        sendInvalidateToOthers(interconnect, sourceId, address);
        updateLineUsage(cache->setList[setIndex], line);
    } else {
        // Cache miss, handle it
        fetchDataFromDirectoryOrCache(cache, address);
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
 * @brief Prepares and sends a read acknowledgment message.
 * 
 * @param interconnect 
 * @param destId 
 * @param address 
 * @param data 
 */
void sendReadResponse(interconnect_t *interconnect, int destId, unsigned long address, data_t *data) {
    // First, determine the current state of the line in the directory
    int index = directoryIndex(address);
    directory_entry_t* directoryLine = &interconnect->nodeList[interconnect->processor_id].directory->lines[index];

    // Check the state and update accordingly
    if (directoryLine->state == DIR_EXCLUSIVE_MODIFIED && directoryLine->owner == interconnect->processor_id) {
        // If the line is exclusively modified by this cache, change state to SHARED
        directoryLine->state = DIR_SHARED;
        // Notify other caches about this change
        notifyStateChangeToShared(interconnect, interconnect->processor_id, address);
    }

    // Prepare the read response message
    message_t readResponse;
    readResponse.type = READ_ACKNOWLEDGE;
    readResponse.sourceId = interconnect->nodeList[interconnect->processor_id].cache->processor_id;
    readResponse.destId = destId;
    readResponse.address = address;
    // readResponse.data = data; // Include data if needed

    // Enqueue the message to the outgoing queue
    enqueue(interconnect->outgoingQueue, readResponse);
}


/**
 * @brief Notifies all caches of a change in the state of a cache line to shared.
 * 
 * @param interconnect 
 * @param cacheId 
 * @param address 
 */
void notifyStateChangeToShared(interconnect_t *interconnect, int cacheId, unsigned long address) {
    // Iterate over all caches and notify them about the state change
    for (int i = 0; i < NUM_PROCESSORS; i++) {
        if (i != cacheId) {
            // Prepare a message for each cache
            message_t stateChangeMsg;
            stateChangeMsg.type = STATE_UPDATE; // Assuming a STATE_UPDATE message type exists
            stateChangeMsg.sourceId = cacheId;
            stateChangeMsg.destId = i;
            stateChangeMsg.address = address;

            // Enqueue the state update message to the outgoing queue
            enqueue(interconnect->outgoingQueue, stateChangeMsg);
        }
    }
}

/**
 * @brief Sends an acknowledgment message after a write operation.
 * 
 * @param interconnect 
 * @param destId 
 * @param address 
 */
void sendWriteAcknowledge(interconnect_t *interconnect, int destId, unsigned long address) {
    // Update the state of the line in the directory
    int index = directoryIndex(address);
    directory_entry_t* directoryLine = &interconnect->nodeList[interconnect->processor_id].directory->lines[index];

    // If this cache is the new owner of the line, update the directory
    if (directoryLine->state != DIR_EXCLUSIVE_MODIFIED || directoryLine->owner != interconnect->processor_id) {
        // Invalidate other caches holding the line
        for (int i = 0; i < NUM_PROCESSORS; i++) {
            if (i != interconnect->processor_id && directoryLine->existsInCache[i]) {
                // Prepare invalidate messages for other caches
                message_t invalidateMsg;
                invalidateMsg.type = INVALIDATE;
                invalidateMsg.sourceId = interconnect->processor_id;
                invalidateMsg.destId = i;
                invalidateMsg.address = address;

                enqueue(interconnect->outgoingQueue, invalidateMsg);
                directoryLine->existsInCache[i] = false;
            }
        }

        // Update the directory to reflect this cache as the exclusive modifier
        directoryLine->state = DIR_EXCLUSIVE_MODIFIED;
        directoryLine->owner = interconnect->processor_id;
        directoryLine->existsInCache[interconnect->processor_id] = true;
    }

    // Prepare the write acknowledge message
    message_t writeAck;
    writeAck.type = WRITE_ACKNOWLEDGE;
    writeAck.sourceId = interconnect->nodeList[interconnect->processor_id].cache->processor_id;
    writeAck.destId = destId;
    writeAck.address = address;

    enqueue(interconnect->outgoingQueue, writeAck);
}


/**
 * @brief Updates the state of a specific cache line.
 * 
 * @param cache 
 * @param address 
 * @param newState 
 * @return true 
 * @return false 
 */
bool updateCacheLineState(cache_t *cache, unsigned long address, block_state newState) {
    unsigned long setIndex = calculateSetIndex(address, cache->S, cache->B);
    unsigned long tag = calculateTag(address, cache->S, cache->B);
    bool lineFound = false;

    for (unsigned int i = 0; i < cache->E; ++i) {
        line_t *line = &cache->setList[setIndex].lines[i];
        if (line->tag == tag && line->valid) {
            line->state = newState;
            lineFound = true;
            break;
        }
    }

    // Additional logic for directory update

    return lineFound;
}

/**
 * @brief Retrieves data from another cache or directory on a cache miss.
 * 
 * @param cache 
 * @param address 
 */
void fetchDataFromDirectoryOrCache(cache_t *cache, unsigned long address) {
    unsigned long setIndex = calculateSetIndex(address, cache->S, cache->B);
    unsigned long tag = calculateTag(address, cache->S, cache->B);
    bool isCacheMiss = true;

    // Check if the data is in the current cache
    for (unsigned int i = 0; i < cache->E; ++i) {
        if (cache->setList[setIndex].lines[i].tag == tag && cache->setList[setIndex].lines[i].valid) {
            isCacheMiss = false;
            break;
        }
    }

    // If cache miss, determine where to fetch the data from
    if (isCacheMiss) {
        int cacheIdWithData = findCacheWithLine(address);

        if (cacheIdWithData != -1) {
            // Data is in another cache, send FETCH message
            message_t fetchMsg = createMessage(FETCH, cache->processor_id, cacheIdWithData, address);
            enqueue(interconnect->outgoingQueue, fetchMsg);
        } else {
            // Data is not in any cache, fetch from directory
            message_t readRequest = createMessage(READ_REQUEST, cache->processor_id, address / NUM_PROCESSORS , address);
            enqueue(interconnect->outgoingQueue, readRequest);
        }
    }
}


/**
 * @brief Displays the structure and contents of a cache for debugging.
 * 
 * @param cache The cache to be printed.
 */
void printCache(cache_t *cache) {
    if (cache == NULL) {
        printf("Cache is NULL\n");
        return;
    }

    printf("Cache Structure (Processor ID: %d)\n", cache->processor_id);
    printf("Total Sets: %lu, Lines per Set: %lu, Block Size: %lu\n", 
           (1UL << cache->S), cache->E, (1UL << cache->B));
    printf("Hit Count: %lu, Miss Count: %lu, Eviction Count: %lu, Dirty Eviction Count: %lu\n", 
           cache->hitCount, cache->missCount, cache->evictionCount, cache->dirtyEvictionCount);

    for (unsigned long i = 0; i < (1UL << cache->S); i++) {
        printf("Set %lu:\n", i);
        for (unsigned long j = 0; j < cache->E; j++) {
            line_t *line = &cache->setList[i].lines[j];
            printf("  Line %lu: Tag: %lx, Valid: %d, Dirty: %d, State: %d, Last Used: %lu\n", 
                   j, line->tag, line->valid, line->isDirty, line->state, line->lastUsed);
        }
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
 * @brief Generates a summary of cache performance statistics.
 * 
 * @param cache 
*/
csim_stats_t *makeSummary(cache_t *cache) {
    if (cache == NULL) {
        printf("Cache is NULL\n");
        return NULL;
    }

    csim_stats_t *stats = malloc(sizeof(csim_stats_t));
    if (stats == NULL) {
        printf("Failed to allocate memory for cache stats\n");
        return NULL;
    }

    stats->hits = cache->hitCount;
    stats->misses = cache->missCount;
    stats->evictions = cache->evictionCount;
    stats->dirtyBytes = 0; // Assuming you have a way to calculate this
    stats->dirtyEvictions = cache->dirtyEvictionCount;

    return stats;
}


