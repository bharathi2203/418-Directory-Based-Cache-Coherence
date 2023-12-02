/**
 * @file single_cache.c
 * @brief Cache Simulator
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
 * @param[in]       s       number of set bits in the address
 * @param[in]       e       number of lines in each set
 * @param[in]       b       number of block bits in the address
 * @param[in]  processor_id processor number to identify cache 
 * @param[out]      new     newly allocated Cache.
 * 
*/
cache_t *initializeCache(unsigned int s, unsigned int e, unsigned int b, int processor_id) {
    unsigned int S = 1U << s;
    if (e == 0) {
        return NULL;
    }
    cache_t *new = malloc(sizeof(cache_t));
    if (new == NULL){
        return NULL;
    }
    new->processor_id = processor_id;
    new->S = s;
    new->E = e;
    new->B = b;
    new->hitCount = 0;
    new->missCount = 0;
    new->evictionCount = 0;
    new->dirtyEvictionCount = 0;

    // Initialize sets
    new->setList = (set_t *)malloc(S * sizeof(set_t));
    if (new->setList == NULL) {
        free(new);
        return NULL;
    }
    for (unsigned int i = 0; i < S; i++) {
        new->setList[i].lines = (line_t *)malloc(e * sizeof(line_t));
        new->setList[i].lruCounter = (unsigned long *)malloc(e * sizeof(unsigned long));
        // Initialize lines 
        for (unsigned int j = 0; j < e; j++) {
            new->setList[i].lines[j].lineNum = j;
            new->setList[i].lines[j].valid = false;
             new->setList[i].lines[j].isDirty = false;
            new->setList[i].lruCounter[j] = 0;
        }
    }

    /*
    Ensure that each cache and directory instance can communicate with 
    the interconnect. This might involve passing a reference to the 
    interconnect to each cache and directory upon their initialization.
    */

    return new; 
}

/**
 * @brief Simulates the execution of an instruction by a processor.
 * 
 * @param[in]   cache       Cache struct for a given processor
 * @param[in]   line        Line from tracefile
 */
void executeInstruction(cache_t *cache, char *line) {
    char instruction;
    unsigned long address;
    int processor_id;  // Used only for write instructions

    sscanf(line, "%d %c %lx", &processor_id, &instruction, &address);

    switch(instruction) {
        case 'R':
            readFromCache(cache, address);
            break;
        case 'W':
            writeToCache(cache, address);
            break;
        default:
            // Invalid instruction type
            break;
    }
}

/**
 * @brief Update the counters to implement LRU 
 * 
 */
void updateLRUCounter(set_t *set, unsigned long lineNum) {
    // Increment all counters
    for (unsigned int i = 0; i < set->maxLines; i++) {
        set->lruCounter[i]++;
    }
    // Reset the counter for the accessed line
    set->lruCounter[lineNum] = 0;
}

/**
 * @brief Handles read operations from the processor's cache.
 * 
 * @param[in]   cache       Cache struct for a given processor
 * @param[in]   address     Address of memory being read
 *
 */
int readFromCache(cache_t *cache, unsigned long address) {
    unsigned long setIndex = (address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = address >> (cache->B + cache->S);
    set_t *set = &cache->setList[setIndex];
    bool hit = false;
    unsigned long hitLineIndex = 0;

    // Check if the address is in cache
    for (unsigned int i = 0; i < set->maxLines; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            // Cache hit
            hit = true;
            hitLineIndex = i;
            break;
        }
    }

    if (hit) {
        cache->hitCount++;
        updateLRUCounter(set, hitLineIndex);
        return 0; 
    } else {
        cache->missCount++;
        cacheMissHandler(cache, address, false); // Handle cache miss
        return 1; 
    }
}


/**
 * @brief Handles write operations to the processor's cache.
 * 
 * @param[in]   cache       Cache struct for a given processor
 * @param[in]   address     Address of memory being read
 *
 * @param[out]  status      Status of the write operation.
 */
int writeToCache(cache_t *cache, unsigned long address) {
    unsigned long setIndex = (address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = address >> (cache->B + cache->S);
    set_t *set = &cache->setList[setIndex];
    bool hit = false;
    unsigned long hitLineIndex = 0;

    // Check if the address is in cache
    for (unsigned int i = 0; i < set->maxLines; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            // Cache hit
            hit = true;
            hitLineIndex = i;
            break;
        }
    }

    if (hit) {
        cache->hitCount++;
        updateLRUCounter(set, hitLineIndex);
        set->lines[hitLineIndex].isDirty = true;
        return 0; // Successful write
    } else {
        cache->missCount++;
        cacheMissHandler(cache, address, true); // Handle cache miss
        // Indicates that a miss occurred and write is pending
        return 1; 
    }
}


/**
 * @brief Manages cache miss scenarios.
 * 
 * @param[in]   cache       Cache struct for a given processor
 * @param[in]   address     Address of memory being read
 * 
 */
int cacheMissHandler(cache_t *cache, unsigned long address, bool isDirty) {
    // Calculate set index and tag from the address
    unsigned long setIndex = (address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = address >> (cache->B + cache->S);

    // Get the corresponding set from the cache
    set_t *set = &cache->setList[setIndex];

    // Variables to track the least recently used line
    unsigned long lruLineIndex = 0;
    unsigned long maxLRUValue = 0;
    bool setFull = true;

    // Check each line in the set to find an empty line or the LRU line
    for (unsigned int i = 0; i < set->maxLines; i++) {
        if (!set->lines[i].valid) {
            // An empty line is found
            setFull = false;
            lruLineIndex = i;
            break;
        } else if (set->lruCounter[i] > maxLRUValue) {
            // Update LRU line information
            maxLRUValue = set->lruCounter[i];
            lruLineIndex = i;
        }
    }

    // If the set is full and the selected line is dirty, write back to memory
    if (setFull && set->lines[lruLineIndex].isDirty) {
        // Write back the dirty line to memory (code not shown)
        // This would involve memory write operations
        cache->dirtyEvictionCount++;
    }

    // Evict the old line if the set is full
    if (setFull) {
        cache->evictionCount++;
    }

    // Update the LRU line with new data
    set->lines[lruLineIndex].tag = tag;
    set->lines[lruLineIndex].valid = true;
    set->lines[lruLineIndex].isDirty = isDirty; // New data is not dirty yet
    set->lines[lruLineIndex].state = SHARED;   // Initially in SHARED state

    // Reset LRU counters
    for (unsigned int i = 0; i < set->maxLines; i++) {
        set->lruCounter[i]++;
    }
    set->lruCounter[lruLineIndex] = 0;

    // Load the new block of data into the cache line
    // This would typically involve fetching data from main memory or other caches

    return 0;
}



/**
 * @brief Function prints every set, every line in the Cache.
 *        Useful for debugging!
 * 
*/
void printCache(cache_t *C) {
    if (C == NULL) {
        printf("Cache is NULL\n");
        return;
    }
    printf("Cache Structure (Processor ID: %d)\n", C->processor_id);
    printf("Total Sets: %lu, Lines per Set: %lu, Block Size: %lu\n", 
           (1UL << C->S), C->E, (1UL << C->B));
    printf("Hit Count: %lu, Miss Count: %lu, Eviction Count: %lu\n", 
           C->hitCount, C->missCount, C->evictionCount);

    for (unsigned long i = 0; i < (1UL << C->S); i++) {
        printf("Set %lu:\n", i);
        for (unsigned long j = 0; j < C->E; j++) {
            line_t *line = &C->setList[i].lines[j];
            printf("  Line %lu: Tag: %lx, Valid: %d, Dirty: %d, State: %d\n", 
                   j, line->tag, line->valid, line->isDirty, line->state);
        }
    }
}




/**
 * @brief Frees all memory allocated to the cache,
 *        including memory allocated after initialization
 *        such as the lines added.
 * 
*/
void freeCache(cache_t *cache) {
    if (cache == NULL) {
        return;
    }
    // Free each set and its constituent structures
    for (unsigned long i = 0; i < (1UL << cache->S); i++) {
        set_t *set = &cache->setList[i];
        // Free the array of lines in each set
        free(set->lines);

        // Free the LRU counter array if it exists
        if (set->lruCounter != NULL) {
            free(set->lruCounter);
        }
    }

    // Free the array of sets
    free(cache->setList);

    // Finally, free the cache itself
    free(cache);
}


/**
 * @brief Makes a "summary" of stats based on the fields of the cache. 
 *        This function is essentially a helper function to create the 
 *        csim_stats_t struct that the printSummary fn requires.
 * 
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


