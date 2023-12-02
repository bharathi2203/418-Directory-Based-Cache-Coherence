/**
 * @file single_cache.h
 * @brief Implement basic LRU cache. 
 */

#ifndef CACHE_H
#define CACHE_H

#include <stdbool.h>
#include <stdlib.h>

/** @brief Number of clock cycles for hit */
#define HIT_CYCLES 4

/** @brief Number of clock cycles for miss */
#define MISS_CYCLES 100


/**
 * @brief The state that a given block can be in.
 * 
 */
typedef enum { INVALID, SHARED, EXCLUSIVE, MODIFIED } block_state;

/**
 * @brief Struct representing each line/block in a cache
 * 
 * Lines are implemented as linked lists.
*/
typedef struct line {
    unsigned long lineNum;     // Index of the line 
    unsigned long tag;          // Represents tag bits
    bool valid;                 // Represents valid bit
    bool isDirty;               // Represents dirty bit for each cache line
    CacheLineState state;       // State of the cache line (MESI)
} line_t;

/**
 * @brief Struct representing each set in a cache
 * 
*/
typedef struct set {
    line_t *lines;                // Array of lines in the set
    unsigned long *lruCounter;    // Array to keep track of LRU order
    unsigned long maxLines;       // Total number of lines in the set
} set_t;


/**
 * @brief Struct representing the cache
 * 
*/

typedef struct cache {
    int processor_id;                         // Processor that this cache belongs to
    interconnect_t* interconnect;             // Pointer to the interconnect

    unsigned long S;                          // Number of set bits
    unsigned long E;                          // Associativity: number of lines per set
    unsigned long B;                          // Number of block bits
    struct set *setList;                      // Array of Sets

    unsigned long hitCount;                   // number of hits
    unsigned long missCount;                  // number of misses
    unsigned long evictionCount;              // number of evictions
    unsigned long dirtyEvictionCount;         // number of evictions of dirty lines
} cache_t; 


// Function declarations
cache_t *initializeCache(unsigned int s, unsigned int e, unsigned int b, int processor_id);



#endif /* CACHE_H */
