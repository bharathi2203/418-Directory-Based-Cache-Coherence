/**
 * @file single_cache.c
 * @brief cache 
 * 
 * @author BHARATHI SRIDHAR <bsridha2@andrew.cmu.edu>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include "single_cache.h"


/**
 * @brief Identifies the cache containing a specific line based on the memory address.
 * 
 * @param address 
 * @return int 
 */
/*
int findCacheWithLine(unsigned long address) {
    int nodeId = address / NUM_PROCESSORS; // Hypothetical way to map address to node ID.
    int lineIndex = directoryIndex(address);
    directory_entry_t* directoryLine = &(interconnect->nodeList[nodeId].directory->lines[lineIndex]);

    // If the line is in EXCLUSIVE or MODIFIED state, that means only one cache has it.
    if (directoryLine->state == DIR_EXCLUSIVE_MODIFIED) {
        return directoryLine->owner;
    }

    // If the line is in SHARED state, we could return any cache that has it,
    // or -1 to indicate it can be fetched from memory.
    if (directoryLine->state == DIR_SHARED) {
        for (int i = 0; i < NUM_PROCESSORS; i++) {
            if (directoryLine->existsInCache[i]) {
                return i; // Return the ID of the first cache found with the line.
            }
        }
    }

    // If the line is UNCACHED or no cache has the line, return -1.
    return -1;
}
*/


