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
 * @brief Create the cache with the parameters parsed. 
 * 
 * @param s                 number of set bits in the address
 * @param e                 number of lines in each set
 * @param b                 number of block bits in the address
 * @param processor_id      processor number to identify cache 
 * @return cache_t*         newly allocated Cache
 */
#include "cache.h"

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
 * @brief Simulates the execution of an instruction by a processor.
 * 
 * @param cache         Cache struct for a given processor
 * @param line          Line from tracefile
 */
void executeInstruction(cache_t *cache, char *line) {
    // Same implementation as you provided; interactions with the directory will occur
    // in readFromCache and writeToCache functions.
}

/**
 * @brief Handles read operations from the processor's cache.
 * 
 * @param cache             Cache struct for a given processor
 * @param address           Address of memory being read
 * @return int 
 */
int readFromCache(cache_t *cache, unsigned long address) {
    // Implementation should check the cache line's state and interact with the directory
    // This is a placeholder for the actual implementation.
    return interactWithDirectory(cache, address, false); // false indicates a read operation
}

/**
 * @brief Handles write operations to the processor's cache.
 * 
 * @param cache             Cache struct for a given processor
 * @param address           Address of memory being written to
 * @return int 
 */
int writeToCache(cache_t *cache, unsigned long address) {
    // Implementation should check the cache line's state and interact with the directory
    // This is a placeholder for the actual implementation.
    return interactWithDirectory(cache, address, true); // true indicates a write operation
}


// Manages cache miss scenarios.
int cacheMissHandler(cache_t *cache, unsigned long address, bool isWrite) {
    // Calculate set index and tag from the address
    unsigned long setIndex = (address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = address >> (cache->B + cache->S);
    set_t *set = &cache->setList[setIndex];

    // Request data from the directory
    directory_fetch(cache, address);

    // Evict if necessary
    int evicted = evictLineIfNeeded(set, tag, isWrite);

    // Allocate a new line with the data
    line_t *line = allocateLine(set, tag, isWrite);

    return evicted;  // Return whether an eviction occurred
}

// Placeholder for a function to interact with the directory.
int interactWithDirectory(cache_t *cache, unsigned long address, bool isWrite) {
    if (isWrite) {
        // Send write request to directory
        directory_write_request(cache, address);
    } else {
        // Send read request to directory
        directory_read_request(cache, address);
    }
    // We assume the directory interaction is handled asynchronously and will
    // eventually update the cache line state via a callback
    return 0;  // Placeholder return
}

// Placeholder for a function to request a cache line from the directory.
int requestCacheLineFromDirectory(cache_t *cache, unsigned long address, bool isDirty) {
    // Implementation for requesting a cache line from the directory
    // Depending on the coherence protocol, the directory will send back the data
    // and the cache will update its state accordingly
    directory_fetch(cache, address);
    return 0;  // Placeholder return
}

// Evicts a line if needed and returns whether an eviction occurred
int evictLineIfNeeded(set_t *set, unsigned long tag, bool isWrite) {
    // Find the least recently used line (LRU) for eviction
    line_t *lruLine = findLRULine(set);
    if (lruLine->valid) {
        if (lruLine->isDirty) {
            // Write back to memory or send data to the directory if dirty
            directory_invalidate_others(set->cache, lruLine->tag);
            // Increment dirty eviction count
            set->cache->dirtyEvictionCount++;
        }
        // Invalidate the line and mark for replacement
        lruLine->valid = false;
        // Increment overall eviction count
        set->cache->evictionCount++;
        return 1;  // An eviction occurred
    }
    return 0;  // No eviction occurred
}

// Allocates a new line for data fetched from the directory
line_t *allocateLine(set_t *set, unsigned long tag, bool isWrite) {
    // Find an invalid line to use or reuse the LRU line
    line_t *newLine = findInvalidOrLRULine(set);
    newLine->tag = tag;
    newLine->valid = true;
    newLine->isDirty = isWrite;
    newLine->state = isWrite ? MODIFIED : SHARED;
    resetLRUCounter(set, newLine);
    return newLine;  // Return the newly allocated line
}

// Resets the LRU counter for a line after it's been accessed
void resetLRUCounter(set_t *set, line_t *line) {
    for (unsigned int i = 0; i < set->associativity; i++) {
        if (&set->lines[i] != line) {
            set->lines[i].lastUsed++;
        }
    }
    line->lastUsed = 0;  // The accessed line's counter is reset
}

// Finds the line with the highest LRU counter (least recently used)
line_t *findLRULine(set_t *set) {
    line_t *lruLine = &set->lines[0];
    for (unsigned int i = 1; i < set->associativity; i++) {
        if (set->lines[i].lastUsed > lruLine->lastUsed) {
            lruLine = &set->lines[i];
        }
    }
    return lruLine;
}


// Initiates a read request to the directory to fetch a cache line.
void directory_read_request(cache_t *cache, unsigned long address) {
    // Assuming the directory API provides a function to handle read requests
    // This function would also handle the logic of what happens when the directory responds
    // It could be an asynchronous call that will trigger a callback function once the data is ready

    // Pseudo-code for sending a read request to the directory
    DirectoryResponse response = DirectoryAPI_sendReadRequest(cache->processor_id, address);

    // Check if the response is immediate or if there's a waiting mechanism
    if (response == DIRECTORY_RESPONSE_PENDING) {
        // If the directory response is pending, the actual read will be handled asynchronously
        // For instance, an event or an interrupt might be triggered when the data is ready
    } else if (response == DIRECTORY_RESPONSE_IMMEDIATE) {
        // If the directory provides the data immediately, update the cache line directly
        updateCacheLineWithData(cache, address, response.data);
    }
}

// Initiates a write request to the directory to propagate a write operation.
void directory_write_request(cache_t *cache, unsigned long address) {
    // Assuming the directory API provides a function to handle write requests
    // This function would also handle the logic of invalidating other caches' lines if needed

    // Pseudo-code for sending a write request to the directory
    DirectoryResponse response = DirectoryAPI_sendWriteRequest(cache->processor_id, address);

    // Handle the response similar to the read request above
    if (response == DIRECTORY_RESPONSE_PENDING) {
        // If the directory response is pending, the actual update will be handled asynchronously
    } else if (response == DIRECTORY_RESPONSE_IMMEDIATE) {
        // If the directory provides immediate confirmation, update the cache line state
        updateCacheLineState(cache, address, MODIFIED);
    }
}

// Function to update a cache line with data from the directory
void updateCacheLineWithData(cache_t *cache, unsigned long address, Data data) {
    // This function will be called once data is received from the directory
    // Update the cache line with the received data
    line_t *line = findCacheLine(cache, address);
    if (line != NULL) {
        line->data = data;  // Assuming the line_t structure has a 'data' field
        line->valid = true;
        line->state = SHARED;  // or EXCLUSIVE depending on the protocol
    }
}

// Function to update the state of a cache line after a write request
void updateCacheLineState(cache_t *cache, unsigned long address, block_state state) {
    // Update the cache line state to reflect the write operation
    line_t *line = findCacheLine(cache, address);
    if (line != NULL) {
        line->state = state;
    }
}

// Utility function to find a cache line based on an address
line_t *findCacheLine(cache_t *cache, unsigned long address) {
    // Derive the set index and tag from the address
    unsigned long setIndex = extractSetIndex(address, cache->B, cache->S);
    unsigned long tag = extractTag(address, cache->B, cache->S);

    // Find the set
    set_t *set = &cache->setList[setIndex];

    // Find the line within the set
    for (unsigned int i = 0; i < set->associativity; ++i) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            return &set->lines[i];
        }
    }

    return NULL;  // Return NULL if the line is not found
}

// Extracts the set index from an address
unsigned long extractSetIndex(unsigned long address, unsigned long B, unsigned long S) {
    return (address >> B) & ((1UL << S) - 1);
}

// Extracts the tag from an address
unsigned long extractTag(unsigned long address, unsigned long B, unsigned long S) {
    return address >> (B + S);
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
    printf("Hit Count: %lu, Miss Count: %lu, Eviction Count: %lu\n", 
           cache->hitCount, cache->missCount, cache->evictionCount);

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
const csim_stats_t *makeSummary(cache_t *C) {
    if (C == NULL) {
        return NULL; // Handle null cache pointer
    }

    csim_stats_t *stats = malloc(sizeof(csim_stats_t));
    if (stats == NULL) {
        return NULL; // Handle failed memory allocation
    }

    stats->hits = C->hitCount;
    stats->misses = C->missCount;
    stats->evictions = C->evictionCount;

    unsigned long dirtyByteCount = 0;
    // Loop through all sets and lines, count all dirty lines
    for (unsigned int i = 0; i < (1UL << C->S); i++) {
        set_t *currSet = &C->setList[i];
        for (unsigned int j = 0; j < C->E; j++) {
            if (currSet->lines[j].valid && currSet->lines[j].isDirty) {
                dirtyByteCount++;
            }
        }
    }
    
    // Calculate dirty bytes: number of dirty lines multiplied by block size
    stats->dirty_bytes = dirtyByteCount * (1UL << C->B);

    // Calculate dirty evictions: number of dirty evictions multiplied by block size
    stats->dirty_evictions = C->dirtyEvictionCount * (1UL << C->B);

    return stats;
}


