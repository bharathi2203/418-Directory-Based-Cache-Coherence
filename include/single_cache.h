/**
 * @file single_cache.h
 * @brief Implement basic LRU cache. 
 */

#ifndef CACHE_H
#define CACHE_H

#include <stdbool.h>
#include <stdlib.h>
#include <interconnect.h>

/** @brief Number of clock cycles for hit */
#define HIT_CYCLES 4

/** @brief Number of clock cycles for miss */
#define MISS_CYCLES 100
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
 * @brief 
 * 
 */
typedef struct {
    unsigned long hits;
    unsigned long misses;
    unsigned long evictions;
    unsigned long dirtyBytes;
    unsigned long dirtyEvictions;
} csim_stats_t;


/**
 * @brief The state that a given block can be in.
 * 
 */
typedef enum {INVALID, SHARED, EXCLUSIVE, MODIFIED} block_state;

/**
 * @brief Struct representing each line/block in a cache
 * 
 * Lines are implemented as linked lists.
*/
typedef struct line {
    unsigned long tag;          // Represents tag bits
    bool valid;                 // Represents valid bit
    bool isDirty;               // Represents dirty bit for each cache line
    block_state state;       // State of the cache line (MESI)
    unsigned long lastUsed;     // LRU counter for the line
} line_t;

/**
 * @brief Struct representing each set in a cache
 * 
*/
typedef struct set {
    line_t *lines;                      // Array of lines in the set
    unsigned long associativity;        // Total number of lines in the set
} set_t;


/**
 * @brief Struct representing the cache
 * 
*/

typedef struct cache {
    int processor_id;                         // Processor that this cache belongs to
    
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
// add other function declarations here


#endif /* CACHE_H */


/**
 * @brief 
 * 
 */
typedef struct {
    unsigned long hits;
    unsigned long misses;
    unsigned long evictions;
    unsigned long dirtyBytes;
    unsigned long dirtyEvictions;
} csim_stats_t;


/**
 * @brief The state that a given block can be in.
 * 
 */
typedef enum {INVALID, SHARED, EXCLUSIVE, MODIFIED} block_state;

/**
 * @brief Struct representing each line/block in a cache
 * 
 * Lines are implemented as linked lists.
*/
typedef struct line {
    unsigned long tag;          // Represents tag bits
    bool valid;                 // Represents valid bit
    bool isDirty;               // Represents dirty bit for each cache line
    CacheLineState state;       // State of the cache line (MESI)
    unsigned long lastUsed;     // LRU counter for the line
} line_t;

/**
 * @brief Struct representing each set in a cache
 * 
*/
typedef struct set {
    line_t *lines;                      // Array of lines in the set
    unsigned long associativity;        // Total number of lines in the set
} set_t;


/**
 * @brief Struct representing the cache
 * 
*/

typedef struct cache {
    int processor_id;                         // Processor that this cache belongs to
    
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
unsigned long calculateSetIndex(unsigned long address, unsigned long S, unsigned long B);
unsigned long calculateTag(unsigned long address, unsigned long S, unsigned long B);
line_t* findLineInSet(set_t set, unsigned long tag);
int findCacheWithLine(unsigned long address);
void processReadRequest(cache_t *cache, int sourceId, unsigned long address);
void processWriteRequest(cache_t *cache, int sourceId, unsigned long address);
void invalidateCacheLine(cache_t *cache, unsigned long address);
void sendReadResponse(interconnect_t *interconnect, int destId, unsigned long address);
void notifyStateChangeToShared(interconnect_t *interconnect, int cacheId, unsigned long address);
void sendWriteAcknowledge(interconnect_t *interconnect, int destId, unsigned long address);
bool updateCacheLineState(cache_t *cache, unsigned long address, block_state newState);
void fetchDataFromDirectoryOrCache(cache_t *cache, unsigned long address);
void printCache(cache_t *cache);
void freeCache(cache_t *cache);


#endif /* CACHE_H */
