/**
 * @file distributed_directory.c
 * @brief Implement a central directory based cache coherence protocol. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "single_cache.h"
#include "distributed_directory.h"
#include "interconnect.h"

/**
 * @brief Global Structs
 * 
 */
directory_t *directory = NULL;
interconnect_t *interconnect = NULL;
cache_t *allCaches[NUM_PROCESSORS];

/**
 * @brief Initialize the directory
 * 
 * @param numLines 
 * @return directory_t* 
 */
directory_t* initializeDirectory(int numLines) {
    directory_t* dir = (directory_t*)malloc(sizeof(directory_t));
    dir->lines = (directory_entry_t*)malloc(sizeof(directory_entry_t) * numLines);
    dir->numLines = numLines;
    pthread_mutex_init(&dir->lock, NULL);
    
    for (int i = 0; i < numLines; i++) {
        dir->lines[i].state = DIR_UNCACHED;
        for(int j = 0; j < NUM_PROCESSORS; j++){
            dir->lines[i].existsInCache[j] = false;
        }
        dir->lines[i].owner = -1;
        pthread_mutex_init(&dir->lines[i].lock, NULL);
    }
    return dir;
}

/**
 * @brief Helper function to find the directory index for a given address
 * 
 * @param address 
 * @return int 
 */
int directoryIndex(int address) {
   return address % NUM_LINES;
}


void freeDirectory(directory_t* dir) {
    if (dir) {
        free(dir->lines);
        free(dir);
    }
}



/**
 * @brief initializeSystem
 * 
 */
void initializeSystem(void) {
    // Initialize the central directory
    directory = initializeDirectory(NUM_LINES);

    // Initialize the interconnect
    interconnect = createInterconnect();

    // Initialize and connect all caches to the interconnect
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        caches[i] = initializeCache(S, E, B, i);  // Initialize cache for processor 'i'
        connectCacheToInterconnect(caches[i], interconnect);
    }

    // Connect the interconnect to the central directory
    connectInterconnectToDirectory(interconnect, directory);

}

/**
 * @brief cleanup System
 * 
 */
void cleanupSystem(void) {
    // Cleanup caches
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        if (caches[i] != NULL) {
            freeCache(caches[i]);
        }
    }

    // Cleanup interconnect
    if (interconnect != NULL) {
        freeInterconnect(interconnect);
    }

    // Finally, free the central directory
    if (directory != NULL) {
        freeDirectory(directory);
    }
}

void fetchFromDirectory(directory_t* directory, int address, int requestingProcessorId) {
    int index = directoryIndex(address);

    // If the line is in another cache and possibly modified, fetch it from there
    if (directory->lines[index].state == DIR_EXCLUSIVE_MODIFIED) {
        int owningProcessorId = directory->lines[index].owner;
        // Invalidate the data in the owning processor's cache
        invalidateCacheLine(allCaches[owningProcessorId], address);
        // Simulate fetching the data from the owning processor's cache to the requesting cache
        transferCacheLine(allCaches[owningProcessorId], allCaches[requestingProcessorId], address);
        // Update directory to reflect that the requesting processor now has the data
        directory->lines[index].owner = requestingProcessorId;
        directory->lines[index].state = DIR_SHARED;
        for (int i = 0; i < NUM_PROCESSORS; i++) {
            directory->lines[index].existsInCache[i] = false;
        }
        directory->lines[index].existsInCache[requestingProcessorId] = true;
    }
    // If the line is uncached or shared, fetch from memory and update directory
    else if (directory->lines[index].state == DIR_UNCACHED || directory->lines[index].state == DIR_SHARED) {
        // Fetch data from memory (not shown here)
        // Update directory to reflect the new state
        directory->lines[index].state = DIR_SHARED;
        directory->lines[index].existsInCache[requestingProcessorId] = true;
    }
}

void readFromCache(cache_t *cache, int address) {
    int index = directoryIndex(address);
    // Check if the address is present in the cache
    if (cache->setList[index].lines[0].valid && cache->setList[index].lines[0].tag == address) {
        // Cache hit: data is present in the cache
        printf("Cache HIT: Processor %d reading address %d\n", cache->processor_id, address);
    } else {
        // Cache miss: attempt to fetch the data from the directory
        printf("Cache MISS: Processor %d reading address %d\n", cache->processor_id, address);
        fetchFromDirectory(directory, address, cache->processor_id);
        // Simulate loading the data into the cache line
        cache->setList[index].lines[0].valid = true;
        cache->setList[index].lines[0].tag = address;
        cache->setList[index].lines[0].state = SHARED;
    }
}

void writeToCache(cache_t *cache, int address) {
    int index = directoryIndex(address);
    // Check if the address is present in the cache
    if (cache->setList[index].lines[0].valid && cache->setList[index].lines[0].tag == address) {
        // Cache hit: data is present in the cache
        printf("Cache HIT: Processor %d writing to address %d\n", cache->processor_id, address);
    } else {
        // Cache miss: attempt to fetch the data from the directory
        printf("Cache MISS: Processor %d writing to address %d\n", cache->processor_id, address);
        fetchFromDirectory(directory, address, cache->processor_id);
        // Simulate loading the data into the cache line
        cache->setList[index].lines[0].valid = true;
        cache->setList[index].lines[0].tag = address;
        cache->setList[index].lines[0].state = MODIFIED;
    }
    // Update the cache line to modified
    cache->setList[index].lines[0].state = MODIFIED;
    // Update the directory to reflect that this processor has modified the data
    directory->lines[index].state = DIR_EXCLUSIVE_MODIFIED;
    directory->lines[index].owner = cache->processor_id;
    for (int i = 0; i < NUM_PROCESSORS; i++) {
        directory->lines[index].existsInCache[i] = false;
    }
    directory->lines[index].existsInCache[cache->processor_id] = true;
}


// Parse a line from the trace file and perform the corresponding operation
void processTraceLine(char *line) {
    int processorId;
    char operation;
    int address;

    // Parse the line to extract the processor ID, operation, and address
    if (sscanf(line, "%d %c %d", &processorId, &operation, &address) == 3) {
        int index = directoryIndex(address);
        switch (operation) {
            case 'R':
                // Perform read operation
                if (!directory->lines[index].existsInCache[processorId]) {
                    // If the line is not in the cache, it's a miss and needs to be loaded
                    readFromCache(allCaches[processorId], address);
                    directory->lines[index].existsInCache[processorId] = true;
                    if (directory->lines[index].state == DIR_UNCACHED) {
                        directory->lines[index].state = DIR_SHARED;
                    }
                }
                // If it's already in the cache, it's a hit and nothing needs to be done
                break;
            case 'W':
                // Perform write operation
                writeFromCache(allCaches[processorId], address);
                // Update the directory to reflect the write operation
                directory->lines[index].state = DIR_EXCLUSIVE_MODIFIED;
                directory->lines[index].owner = processorId;
                // Invalidate other caches
                for (int i = 0; i < NUM_PROCESSORS; i++) {
                    if (i != processorId) {
                        directory->lines[index].existsInCache[i] = false;
                        // Optionally, send an invalidate message to the caches
                    }
                }
                break;
            default:
                fprintf(stderr, "Unknown operation '%c' in trace file\n", operation);
                break;
        }
    } else {
        fprintf(stderr, "Invalid line format in trace file: %s\n", line);
    }
}

/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tracefile>\n", argv[0]);
        return 1;
    }

    // Initialize the system
    initializeSystem();

    // Open the trace file
    FILE *traceFile = fopen(argv[1], "r");
    if (traceFile == NULL) {
        perror("Error opening trace file");
        return 1;
    }

    // Process each line in the trace file
    char line[1024];
    while (fgets(line, sizeof(line), traceFile)) {
        // Here you should parse the line and service the instruction
        // For example:
        // parseLine(line);
        // serviceInstruction(parsedInstruction, interconnect);
    }

    // Cleanup and close the file
    fclose(traceFile);

    // Cleanup resources
    cleanupSystem();

    return 0;
}