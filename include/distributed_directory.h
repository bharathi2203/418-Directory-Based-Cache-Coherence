/**
 * @file distributed_directory.h
 * @brief Implement a central directory based cache coherence protocol. 
 */

#ifndef DISTRIBUTED_DIRECTORY_H
#define DISTRIBUTED_DIRECTORY_H

#include <utils.h> 
// Function declarations for directory
void fetchFromDirectory(directory_t* directory, int address, int requestingProcessorId);


line_t *findLineInSet(set_t *set, unsigned long address);
void addLineToCacheSet(cache_t *cache, set_t *set, unsigned long address, block_state state);
void updateLineUsage(line_t *line);
void updateDirectory(directory_t* directory, unsigned long address, int cache_id, directory_state newState);
void updateDirectoryState(directory_t* directory, unsigned long address, directory_state newState);
void sendReadData(interconnect_t* interconnect, int destId, unsigned long address, bool exclusive);
void sendFetch(interconnect_t* interconnect, int destId, unsigned long address);
void processTraceLine(char *line, interconnect_t* interconnect);
void freeDirectory(directory_t* dir);

// Function declarations for benchmarking / testing 
// void displayUsage(void);

#endif // DISTRIBUTED_DIRECTORY_H
