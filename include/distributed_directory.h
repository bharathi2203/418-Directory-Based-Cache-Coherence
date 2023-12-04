/**
 * @file distributed_directory.h
 * @brief Implement a central directory based cache coherence protocol. 
 */

#ifndef DISTRIBUTED_DIRECTORY_H
#define DISTRIBUTED_DIRECTORY_H

#include <stdbool.h>
#include <pthread.h>

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
    pthread_mutex_t lock; // Mutex for synchronizing access to this entry
} directory_entry_t;

typedef struct {
    directory_entry_t* lines;
    int numLines;
    pthread_mutex_t lock; // Mutex for synchronizing access to the directory
    interconnect_t* interconnect;  // Pointer to the interconnect
} directory_t;

// Function declarations for directory
directory_t* initializeDirectory();
void updateDirectoryEntry(directory_t* directory, int address, int processorId, directory_state newState);
void invalidateDirectoryEntry(directory_t* directory, int address);
void addProcessorToEntry(directory_t* directory, int address, int processorId);
void removeProcessorFromEntry(directory_t* directory, int address, int processorId);
void broadcastInvalidate(directory_t* directory, int address);
void freeDirectory(directory_t* directory);

// Function declarations for directory based protocol
void initializeSystem(void);

// Function declarations for benchmarking / testing 
void displayUsage(void);

#endif // DISTRIBUTED_DIRECTORY_H
