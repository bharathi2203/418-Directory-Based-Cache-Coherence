/**
 * @file distributed_directory.h
 * @brief Implement a central directory based cache coherence protocol. 
 */

#ifndef DISTRIBUTED_DIRECTORY_H
#define DISTRIBUTED_DIRECTORY_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_PROCESSORS 4  
#define NUM_LINES 256

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

// Function declarations for directory
directory_t* initializeDirectory(int numLines);
int directoryIndex(int address);
unsigned long getCurrentTime();
void fetchFromDirectory(directory_t* directory, int address, int requestingProcessorId);
void readFromCache(cache_t *cache, int address);
void writeToCache(cache_t *cache, int address);
line_t *findLineInSet(set_t *set, unsigned long address);
void addLineToCacheSet(set_t *set, unsigned long address, block_state state);
void updateLineUsage(set_t *set, line_t *line);
void updateDirectory(directory_t* directory, unsigned long address, int cache_id, directory_state newState);
void updateDirectoryState(directory_t* directory, unsigned long address, directory_state newState);
void sendReadData(interconnect_t* interconnect, int destId, unsigned long address, bool exclusive);
void sendInvalidate(interconnect_t* interconnect, int destId, unsigned long address);
void sendFetch(interconnect_t* interconnect, int destId, unsigned long address);
void processTraceLine(char *line, interconnect_t* interconnect);
void freeDirectory(directory_t* dir);

// Function declarations for directory based protocol
void initializeSystem(void);
void cleanupSystem(void);

// Function declarations for benchmarking / testing 
// void displayUsage(void);

#endif // DISTRIBUTED_DIRECTORY_H
