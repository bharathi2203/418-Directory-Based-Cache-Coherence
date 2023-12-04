/**
 * @file limited_pointer_dir.h
 * @brief Implement a central directory based cache coherence protocol. 
 */

#ifndef LIMITED_POINTER_DIR_H
#define LIMITED_POINTER_DIR_H

#include <stdbool.h>
#include <pthread.h>
#include <directory.h> 

#define NUM_POINTERS 10

// Directory entry for each block in the main memory
typedef struct {
    directory_state state; // Q: Does each cache need to track the state too?  
    int nodes[NUM_POINTERS]; // Which node has this line 
    int owner; // Owner of the line if in exclusive/modified state
    pthread_mutex_t lock; // Mutex for synchronizing access to this entry
    int numSharedBy; // number of nodes this line is shared by
} lp_directory_entry_t;

typedef struct {
    lp_directory_entry_t* lines;
    int numLines;
    pthread_mutex_t lock; // Mutex for synchronizing access to the directory
    interconnect_t* interconnect;  // Pointer to the interconnect
} lp_directory_t;

// Function declarations for directory
lp_directory_t* initializeDirectory();
void updateDirectoryEntry();
bool checkCacheConsistency();
void freeDirectory(directory_t* directory);

// Function declarations for directory based protocol
void initializeSystem(void);

// Function declarations for benchmarking / testing 
void displayUsage(void);

#endif // LIMITED_POINTER_DIR_H
