/**
 * @file single_cache.c
 * @brief cache 
 * 
 * @author BHARATHI SRIDHAR <bsridha2@andrew.cmu.edu>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

unsigned long calculateSetIndex(unsigned long address, unsigned long S, unsigned long B) {
    return (address >> B) & ((1UL << S) - 1);
}

unsigned long calculateTag(unsigned long address, unsigned long S, unsigned long B) {
    return address >> (S + B);
}

line_t* findLineInSet(set_t set, unsigned long tag) {
    for (unsigned int i = 0; i < set.associativity; ++i) {
        if (set.lines[i].tag == tag && set.lines[i].valid) {
            return &set.lines[i];
        }
    }
    return NULL;  // Line not found
}

int findCacheWithLine(unsigned long address) {
    // This would typically involve querying the directory or the interconnect.
    // Return the ID of the cache that has the line, or -1 if not found.
    // For now, this is just a placeholder.
    return -1;
}



/**
 * @brief 
 * 
 * @param cache 
 * @param sourceId 
 * @param address 
 */
void processReadRequest(cache_t *cache, int sourceId, unsigned long address) {
    // Calculate set index and tag from the address
    unsigned long setIndex = calculateSetIndex(address, cache->S, cache->B);
    unsigned long tag = calculateTag(address, cache->S, cache->B);
    
    // Search for the cache line in the set
    line_t *line = findLineInSet(cache->setList[setIndex], tag);
    
    if (line != NULL && line->valid) {
        // If cache line is found and valid
        if (line->state == SHARED || line->state == EXCLUSIVE) {
            // If line is in a shareable state
            sendReadResponse(interconnect, sourceId, address, line->data); // Assuming line has data
        }
    } else {
        // If cache miss
        fetchDataFromDirectoryOrCache(cache, address); // Function to handle cache miss
    }
}

/**
 * @brief 
 * 
 * @param cache 
 * @param sourceId 
 * @param address 
 */
void processWriteRequest(cache_t *cache, int sourceId, unsigned long address) {
    unsigned long setIndex = (address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = address >> (cache->B + cache->S);

    for (unsigned int i = 0; i < cache->E; ++i) {
        line_t *line = &cache->setList[setIndex].lines[i];
        if (line->tag == tag && line->valid) {
            // Invalidate other caches if necessary
            if (line->state == SHARED || line->state == EXCLUSIVE) {
                // sendInvalidate(interconnect, /* other cache IDs */, address);
            }
            line->state = MODIFIED;
            line->isDirty = true;
            return;
        }
    }

    // If line not found in cache or invalid, fetch it from directory/cache
    fetchDataFromDirectoryOrCache(cache, address);
}

/**
 * @brief 
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
            line->valid = false;
            line->state = INVALID;
            // If line is dirty, handle dirty data (e.g., write back to memory)
            if (line->isDirty) {
                // writeBackToMemory(address, line->data); // Assuming function exists
                line->isDirty = false;
            }
            return;
        }
    }
}


/**
 * @brief 
 * 
 * @param interconnect 
 * @param destId 
 * @param address 
 * @param exclusive 
 * @param data 
 */
void sendReadResponse(interconnect_t *interconnect, int destId, unsigned long address, bool exclusive, data_t *data) {
    message_t readResponse;
    readResponse.type = READ_ACKNOWLEDGE;
    readResponse.sourceId = interconnect->nodeList[/* your cache ID */].cache->processor_id;
    readResponse.destId = destId;
    readResponse.address = address;
    // Assuming data is encapsulated in the message
    // readResponse.data = data; // or a pointer/reference to data

    enqueue(interconnect->outgoingQueue, readResponse);
}

/**
 * @brief 
 * 
 * @param interconnect 
 * @param destId 
 * @param address 
 */
void sendWriteAcknowledge(interconnect_t *interconnect, int destId, unsigned long address) {
    message_t writeAck;
    writeAck.type = WRITE_ACKNOWLEDGE;
    writeAck.sourceId = interconnect->nodeList[/* your cache ID */].cache->processor_id;
    writeAck.destId = destId;
    writeAck.address = address;

    enqueue(interconnect->outgoingQueue, writeAck);
}

/**
 * @brief 
 * 
 * @param cache 
 * @param address 
 * @param newState 
 * @return true 
 * @return false 
 */
bool updateCacheLineState(cache_t *cache, unsigned long address, block_state newState) {
    unsigned long setIndex = (address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = address >> (cache->B + cache->S);

    for (unsigned int i = 0; i < cache->E; ++i) {
        line_t *line = &cache->setList[setIndex].lines[i];
        if (line->tag == tag && line->valid) {
            line->state = newState;
            return true;  // Update successful
        }
    }

    // Line not found, consider adding new line or logging error
    return false;  // Update unsuccessful
}


/**
 * @brief 
 * 
 * @param cache 
 * @param address 
 */
void fetchDataFromDirectoryOrCache(cache_t *cache, unsigned long address) {
    unsigned long setIndex = (address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = address >> (cache->B + cache->S);
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
        // Assuming a function that checks if the data is in another cache
        int cacheIdWithData = findCacheWithLine(address); // This might involve interconnect queries

        if (cacheIdWithData != -1) {
            // Data is in another cache, send FETCH message
            message_t fetchMsg;
            fetchMsg.type = FETCH;
            fetchMsg.sourceId = cache->processor_id;
            fetchMsg.destId = cacheIdWithData;
            fetchMsg.address = address;

            enqueue(interconnect->outgoingQueue, fetchMsg);
        } else {
            // Data is not in any cache, fetch from directory
            message_t readRequest;
            readRequest.type = READ_REQUEST;
            readRequest.sourceId = cache->processor_id;
            readRequest.destId = /* ID of the directory node */;
            readRequest.address = address;

            enqueue(interconnect->outgoingQueue, readRequest);
        }
    }
}



/**
 * @brief Function prints every set, every line in the Cache.
 *        Useful for debugging!
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
 * @brief Frees all memory allocated to the cache,
 *        including memory allocated after initialization
 *        such as the lines added.
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
 * @brief Makes a "summary" of stats based on the fields of the cache. 
 *        This function is essentially a helper function to create the 
 *        csim_stats_t struct that the printSummary fn requires.
 * 
 * @param cache 
*/
const csim_stats_t *makeSummary(cache_t *cache) {
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


