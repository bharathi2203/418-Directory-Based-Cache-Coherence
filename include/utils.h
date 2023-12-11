/**
 * @file utils.h
 */

#ifndef DATA_DEF_H
#define DATA_DEF_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <queue.h>
#include <string.h>
#include <limits.h>

#define NUM_PROCESSORS 4  
#define NUM_LINES 256

#define main_S 1
#define main_E 16
#define main_B 16

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


// States for a cache line for a directory based approach 
typedef enum {
    DIR_UNCACHED,
    DIR_SHARED,
    DIR_EXCLUSIVE_MODIFIED
} directory_state;

// Directory entry for each block in the main memory
typedef struct {
    directory_state state;
    bool existsInCache[NUM_PROCESSORS]; // Presence bits for each cache
    int owner; // Owner of the line if in exclusive/modified state
} directory_entry_t;

typedef struct {
    directory_entry_t* lines;
    int numLines;
} directory_t;

/*
The `message_type` enum defines different types of messages used for communication between caches and memory in the cache coherence system:

1. **READ_REQUEST** (Cache to Memory): 
        Used when a cache requests data from memory.
2. **READ_ACKNOWLEDGE** (Memory to Cache): 
        Sent from memory to cache to acknowledge a READ_REQUEST.
3. **INVALIDATE** (Memory to Cache): 
        Instructs a cache to invalidate a specific cache line.
4. **INVALIDATE_ACK** (Cache to Memory): 
        Cache's response to an INVALIDATE message, acknowledging that the line has been invalidated.
5. **STATE_UPDATE** (Cache to Cache): 
        Communicates state changes of cache lines between caches.
6. **WRITE_REQUEST** (Cache to Memory): 
        Issued when a cache needs to write data to memory.
7. **WRITE_ACKNOWLEDGE** (Memory to Cache): 
        Memory's response to a WRITE_REQUEST, confirming the write operation.

*/
typedef enum {
    READ_REQUEST,       // Cache to Memory
    READ_ACKNOWLEDGE,   // Memory to Cache
    INVALIDATE,         // Memory to Cache
    INVALIDATE_ACK,     // Cache to Memory
    STATE_UPDATE,       // Cache to Cache 
    WRITE_REQUEST,      // Cache to Memory
    WRITE_ACKNOWLEDGE,  // Memory to Cache

    FETCH               // unsure if it need it, maybe removed it earlier?
} message_type;

typedef struct {
    message_type type;
    int sourceId;
    int destId;
    unsigned long address;
} message_t;

typedef struct node {
    directory_t *directory;
    cache_t *cache;
} node_t;

typedef struct interconnect {
    Queue* incomingQueue;  // Queue for incoming messages
    Queue* outgoingQueue;  // Queue for outgoing messages
    node_t nodeList[NUM_PROCESSORS];
} interconnect_t;

/**
 * @brief Global Structs
 * 
 */
extern interconnect_t *interconnect;
extern csim_stats_t *system_stats;

void updateLineUsage(line_t *line);
int directoryIndex(unsigned long address);
void sendInvalidate(int srcID, int destId, unsigned long address);
unsigned long getCurrentTime();
unsigned long calculateTag(unsigned long address, unsigned long S, unsigned long B);
directory_t* initializeDirectory(int numLines);
cache_t *initializeCache(unsigned int s, unsigned int e, unsigned int b, int processor_id);
void freeDirectory(directory_t* dir);
void freeCache(cache_t *cache);
void invalidateCacheLine(cache_t *cache, unsigned long address);
line_t* findLineInSet(set_t set, unsigned long tag);
void addLineToCacheSet(cache_t *cache, set_t *set, unsigned long address, block_state state);
void updateDirectory(directory_t* directory, unsigned long address, int cache_id, directory_state newState);
unsigned long calculateSetIndex(unsigned long address, unsigned long S, unsigned long B);

#endif // DATA_DEF_H
