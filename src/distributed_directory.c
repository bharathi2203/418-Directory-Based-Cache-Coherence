/**
 * @file distributed_directory.c
 * @brief Implement a central directory based cache coherence protocol. 
 */

#include "distributed_directory.h"
#include "interconnect.h"
/**
 * @brief Global Structs
 * 
 */
interconnect_t *interconnect = NULL;
csim_stats_t *system_stats = NULL;





/**
 * @brief Fetches a cache line from the directory for a requesting processor. 
 *
 * This function handles fetching a cache line from the directory based on its 
 * current state and updates its state accordingly. If the cache line is exclusively 
 * modified, it fetches the line from the owning cache and invalidates it there. 
 *
 * For uncached or shared lines, it fetches the line directly from the main memory. 
 * Finally, it updates the directory entry, setting the line to shared state 
 * and recording its presence in the requesting processor's cache.
 * 
 * @param directory                 The directory from which the cache line is fetched.
 * @param address                   The memory address of the cache line to fetch.
 * @param requestingProcessorId     The ID of the processor requesting the cache line.
 */
void fetchFromDirectory(directory_t* directory, int address, int requestingProcessorId) {
    int index = directoryIndex(address);
    directory_entry_t* line = &directory->lines[index];

    // Check the state of the directory line
    if (line->state == DIR_EXCLUSIVE_MODIFIED) {
        // If the line is exclusively modified, we need to fetch it from the owning cache
        int owner = line->owner;
        if (owner != -1) {
            // Invalidate the line in the owner's cache
            sendInvalidate(interconnect, owner, address);

            // Simulate transferring the line to the requesting cache
            sendFetch(interconnect, requestingProcessorId, address);
        }
    } else if (line->state == DIR_UNCACHED || line->state == DIR_SHARED) {
        // If the line is uncached or shared, fetch it from the memory
        sendReadData(interconnect, requestingProcessorId, address, (line->state == DIR_UNCACHED));
    }

    // Update the directory entry
    line->state = DIR_SHARED;
    line->owner = -1;
    for (int i = 0; i < NUM_PROCESSORS; i++) {
        line->existsInCache[i] = false;
    }
    line->existsInCache[requestingProcessorId] = true;
}

/**
 * @brief Adds a line to the cache set using the LRU replacement policy
 * 
 * @param set 
 * @param address 
 * @param state 
 */
void addLineToCacheSet(cache_t *cache, set_t *set, unsigned long address, block_state state) {
    unsigned long tag = address >> (main_S + main_B);
    line_t *oldestLine = NULL;
    unsigned long oldestTime = ULONG_MAX;

    // Look for an empty line or the least recently used line
    for (unsigned int i = 0; i < set->associativity; ++i) {
        line_t *line = &set->lines[i];

        if (!line->valid || line->lastUsed < oldestTime) {
            oldestLine = line;
            oldestTime = line->lastUsed;
        }
    }

    if (oldestLine == NULL) {
        return;
    }

    // Evict the oldest line if necessary
    if (oldestLine->valid && oldestLine->isDirty) {
        // Write back to memory if dirty
        cache->evictionCount++;
        if (oldestLine->isDirty) {
            cache->dirtyEvictionCount++;
        }
    }

    // Update the line with new data
    oldestLine->tag = tag;
    oldestLine->valid = true;
    oldestLine->isDirty = (state == MODIFIED);
    oldestLine->state = state;
    oldestLine->lastUsed = getCurrentTime(); 
}


/**
 * @brief This function updates the directory after a cache line write
 * 
 * @param directory 
 * @param address 
 * @param cache_id 
 * @param newState 
 */
void updateDirectory(directory_t* directory, unsigned long address, int cache_id, directory_state newState) {
    int index = directoryIndex(address);
    directory_entry_t* line = &directory->lines[index];

    // Update the directory entry based on the new state
    line->state = newState;
    line->owner = (newState == DIR_EXCLUSIVE_MODIFIED) ? cache_id : -1;

    // Invalidate other caches if necessary
    if (newState == DIR_EXCLUSIVE_MODIFIED) {
        for (int i = 0; i < NUM_PROCESSORS; i++) {
            if (i != cache_id) {
                line->existsInCache[i] = false;
                sendInvalidate(interconnect, i, address);
            }
        }
    }
}

/**
 * @brief Updates the state of a cache line in the directory.
 * 
 * @param directory The directory containing the cache line.
 * @param address The address of the cache line to update.
 * @param newState The new state to set for the cache line.
 */
void updateDirectoryState(directory_t* directory, unsigned long address, directory_state newState) {
    if (!directory) return; // Safety check

    int index = directoryIndex(address); // Get the index of the cache line in the directory
    directory_entry_t* line = &directory->lines[index]; // Access the directory line

    // Update the state of the directory line
    line->state = newState;

    // Reset owner and presence bits if the state is DIR_UNCACHED
    if (newState == DIR_UNCACHED) {
        line->owner = -1;
        for (int i = 0; i < NUM_PROCESSORS; i++) {
            line->existsInCache[i] = false;
        }
    }
}


/**
 * @brief Sends a read data acknowledgment message via the interconnect system. 
 * 
 * It creates a READ_ACKNOWLEDGE message specifying the source ID (often the directory's 
 * ID), the destination cache ID, and the memory address of the data. The message is 
 * then enqueued in the interconnect's outgoing queue, signaling that the requested data 
 * is ready to be sent to the requesting cache node.
 * 
 * @param interconnect 
 * @param destId 
 * @param address 
 * @param exclusive 
 */
void sendReadData(interconnect_t* interconnect, int destId, unsigned long address, bool exclusive) {
    // Create a READ_ACKNOWLEDGE message
    message_t readDataMsg = {
        .type = READ_ACKNOWLEDGE,
        .sourceId = -1, // -1 or a specific ID if the directory has an ID
        .destId = destId,
        .address = address
    };

    // Enqueue the message to the outgoing queue
    enqueue(interconnect->outgoingQueue, &readDataMsg);
}

/**
 * @brief Facilitates fetching data from a cache node in response to a cache miss in another node. 
 * 
 * On encountering a cache miss, this function is used to create and send a FETCH message 
 * through the interconnect system to the cache node that currently holds the required data. 
 * This ensures efficient data retrieval and consistency across the distributed cache system.
 * 
 * @param interconnect 
 * @param destId 
 * @param address 
 */
void sendFetch(interconnect_t* interconnect, int destId, unsigned long address) {
    // Create a FETCH message
    message_t fetchMsg = {
        .type = FETCH,
        .sourceId = -1, // -1 or a specific ID if the directory has an ID
        .destId = destId,
        .address = address
    };

    // Enqueue the message to the outgoing queue
    enqueue(interconnect->outgoingQueue, &fetchMsg);
}


/**
 * @brief Parse a line from the trace file and perform the corresponding operation
 * 
 * @param line 
 * @param interconnect 
 */
void processTraceLine(char *line, interconnect_t* interconnect) {
    int processorId;
    char operation;
    int address;

    message_t msg;

    // Parse the line to extract the processor ID, operation, and address
    if (sscanf(line, "%d %c %x", &processorId, &operation, &address) == 3) {
        // Convert the address to the local node's index, assuming the address includes the node information
        int localNodeIndex = address / NUM_LINES;
        switch (operation) {
            case 'R':
                // Send a read request to the interconnect queue
                msg.type = READ_REQUEST;
                msg.sourceId = processorId;
                msg.destId = localNodeIndex;
                msg.address = address;
                enqueue(interconnect->incomingQueue, &msg);
                break;
            case 'W':
                // Send a write request to the interconnect queue
                msg.type = WRITE_REQUEST;
                msg.sourceId = processorId;
                msg.destId = localNodeIndex;
                msg.address = address;
                enqueue(interconnect->incomingQueue, &msg);
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
    createInterconnect(NUM_LINES, main_S, main_E, main_B);

    // Open the trace file
    FILE *traceFile = fopen(argv[1], "r");
    if (traceFile == NULL) {
        perror("Error opening trace file");
        return 1;
    }

    // Process each line in the trace file
    char line[1024];
    while (fgets(line, sizeof(line), traceFile)) {
        // Here you we call parse the line and service the instruction
        
    }

    // Cleanup and close the file
    fclose(traceFile);

    // Cleanup resources
    freeInterconnect();

    return 0;
}