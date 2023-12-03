/**
 * @file central_directory.c
 * @brief Implement a central directory based cache coherence protocol. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "single_cache.h"
#include "central_directory.h"
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
   return address % NUM_LINES; // IS THIS CORRECT
}

/**
 * @brief Update the directory entry for a given address
 * 
 * @param directory 
 * @param address 
 * @param processorId 
 * @param newState 
 */
void updateDirectoryEntry(directory_t* directory, int address, int processorId, directory_state newState) {
    int index = directoryIndex(address);
    pthread_mutex_lock(&directory->lines[index].lock);
    directory->lines[index].state = newState;
    directory->lines[index].owner = (newState == DIR_EXCLUSIVE_MODIFIED) ? processorId : -1;
    pthread_mutex_unlock(&directory->lines[index].lock);
}

/**
 * @brief Invalidate the directory entry for a given address
 * 
 * @param directory 
 * @param address 
 */
void invalidateDirectoryEntry(directory_t* directory, int address) {
   int index = directoryIndex(address);
   pthread_mutex_lock(&directory->lines[index].lock);
   directory->lines[index].state = DIR_UNCACHED;
   for(int j = 0; j < NUM_PROCESSORS; j++){
      directory->lines[index].existsInCache[j] = false;
   }
   directory->lines[index].owner = -1;
   pthread_mutex_unlock(&directory->lines[index].lock);
}

/**
 * @brief Add a processor to the directory entry's existsInCache bits
 * 
 * @param directory 
 * @param address 
 * @param processorId 
 */
void addProcessorToEntry(directory_t* directory, int address, int processorId) {
   int index = directoryIndex(address);
   pthread_mutex_lock(&directory->lines[index].lock);
   directory->lines[index].existsInCache[processorId] = true;
   pthread_mutex_unlock(&directory->lines[index].lock);
}

/**
 * @brief Remove a processor from the directory entry's existsInCache bits
 * 
 * @param directory 
 * @param address 
 * @param processorId 
 */
void removeProcessorFromEntry(directory_t* directory, int address, int processorId) {
   int index = directoryIndex(address);
   pthread_mutex_lock(&directory->lines[index].lock);
   directory->lines[index].existsInCache[processorId] = false;
   pthread_mutex_unlock(&directory->lines[index].lock);
}

/**
 * @brief Broadcast an invalidate to all processors except the owner for a given address
 * 
 * @param directory 
 * @param address 
 */
void broadcastInvalidate(directory_t* directory, int address) {
    int index = directoryIndex(address);
    pthread_mutex_lock(&directory->lines[index].lock);
    if(directory->lines[index].state != DIR_UNCACHED) {
        // Send invalidate message to all processors except the owner
        for(int j = 0; j < NUM_PROCESSORS; j++){
            if(j != directory->lines[index].owner){
                // sendMessage(INVALIDATE, address, j);
            }
        }
    }
    pthread_mutex_unlock(&directory->lines[index].lock);
}


// Check if the cache is in a consistent state with the directory
bool checkCacheConsistency(directory_t* directory, int lineIndex, int processorId) {
   // Implement consistency checking logic here
   return true; // placeholder return
}


/**
 * @brief Free the directory
 * 
 * @param dir 
 */
void freeDirectory(directory_t* dir) {
   if(dir != NULL) {
      if(dir->lines != NULL) {
         for(int i = 0; i < dir->numLines; i++) {
            pthread_mutex_destroy(&dir->lines[i].lock);
         }
         free(dir->lines);
      }
      pthread_mutex_destroy(&dir->lock);
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

/**
 * @brief 
 * 
 * @param request_line 
 */
void executeInstruction(char *request_line) {
   // Implement instruction execution logic...
}

/**
 * @brief Prints information about what parameters the program requires and it's format.
 * 
*/
void displayUsage(void) {
   return;
}

/**
 * @brief 
 * 
 * @param arg 
 */
void interconnectProcessMessages(void *arg) {
    interconnect_t *interconnect = (interconnect_t *)arg;
    if (interconnect == NULL || interconnect->queue == NULL) return;

    while (true) {
        pthread_mutex_lock(&interconnect->mutex);

        if (isQueueEmpty(interconnect->queue)) {
            pthread_mutex_unlock(&interconnect->mutex);
            // consider adding a wait condition here
            continue;
        }

        message_t *message = (message_t *)dequeue(interconnect->queue);
        pthread_mutex_unlock(&interconnect->mutex);

        if (message) {
            switch(message->type) {
                case READ_REQUEST:
                    // Handle Read Request (RR)
                    // Direct this request to the memory, which will have to fetch the data
                    handleReadRequest(message);
                    break;
                case READ_ACKNOWLEDGE:
                    // Handle Read Acknowledge (RA)
                    // This message would come from the memory to the cache
                    handleReadAcknowledge(message);
                    break;
                case INVALIDATE:
                    // Handle Invalidate (IV)
                    // This message can come from memory or another cache
                    handleInvalidate(message);
                    break;
                case INVALIDATE_ACK:
                    // Handle Invalidate Acknowledge (IA)
                    // This message confirms an invalidate operation
                    handleInvalidateAcknowledge(message);
                    break;
                case WRITE_REQUEST:
                    // Handle Write Request (WR)
                    // Direct this request to the memory, which will update the data
                    handleWriteRequest(message);
                    break;
                case WRITE_UPDATE:
                    // Handle Write Update (WU)
                    // This message would update the data in the cache
                    handleWriteUpdate(message);
                    break;
                case WRITE_ACKNOWLEDGE:
                    // Handle Write Acknowledge (WA)
                    // This message confirms a write operation
                    handleWriteAcknowledge(message);
                    break;
                case UPDATE:
                    // Handle Update (UD)
                    // This message updates the cache line with new data
                    handleUpdate(message);
                    break;
                default:
                    // Handle unknown message type
                    fprintf(stderr, "Unknown message type received\n");
                    break;
            }

            free(message); // Free the dequeued message
        }
    }
}

/**
 * @brief 
 * 
 * @param message 
 * @param interconnect 
 * @param directory 
 */
void handleReadRequest(message_t *message, interconnect_t *interconnect, directory_t *directory) {
    // Lock the directory entry for the given address
    int dirIndex = directoryIndex(message->address);
    pthread_mutex_lock(&directory->lines[dirIndex].lock);

    // Check the state of the directory entry
    directory_entry_t *dirEntry = &directory->lines[dirIndex];
    if (dirEntry->state == DIR_UNCACHED) {
        // The data is not cached, read from memory and send READ_ACKNOWLEDGE to the requester
        // Here, simulate memory read by just setting the data to zero for example
        int data = 0; // Replace this with actual memory read if applicable

        // Send READ_ACKNOWLEDGE message back to the requester
        message_t replyMessage;
        replyMessage.type = READ_ACKNOWLEDGE;
        replyMessage.sourceId = -1; // Memory ID, assuming -1 represents memory
        replyMessage.destId = message->sourceId;
        replyMessage.address = message->address;
        // Add data to replyMessage if the protocol requires it

        interconnectSendMessage(interconnect, replyMessage);

        // Update the directory entry to reflect the new state
        dirEntry->state = DIR_SHARED;
        dirEntry->owner = message->sourceId;
        dirEntry->existsInCache[message->sourceId] = true;
    } else if (dirEntry->state == DIR_SHARED || dirEntry->state == DIR_EXCLUSIVE_MODIFIED) {
        // If the data is already in shared state or exclusively modified by another cache,
        // decide what to do based on the specific protocol
        // For example, if it's in shared state, just send the data to the requester
        // If it's exclusively modified, invalidate the owner and transfer ownership

        // Simulated action: let's say we just send the data as is
        message_t replyMessage;
        replyMessage.type = READ_ACKNOWLEDGE;
        replyMessage.sourceId = dirEntry->owner; // The current owner of the data
        replyMessage.destId = message->sourceId;
        replyMessage.address = message->address;
        // Add data to replyMessage if the protocol requires it

        interconnectSendMessage(interconnect, replyMessage);

        // Add the requester to the list of caches that have this data
        dirEntry->existsInCache[message->sourceId] = true;
    }

    // Unlock the directory entry
    pthread_mutex_unlock(&directory->lines[dirIndex].lock);
}


/**
 * @brief 
 * 
 * @param message 
 * @param cache 
 */
void handleReadAcknowledge(message_t *message, cache_t *cache) {
    // Convert the address in the message to a cache set index and tag
    unsigned long setIndex = (message->address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = message->address >> (cache->B + cache->S);

    // Find the set in the cache where this address should be
    set_t *set = &cache->setList[setIndex];

    // Check if the address is already in the cache
    for (unsigned int i = 0; i < set->maxLines; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            // The line is already in the cache, update the data
            set->lines[i].isDirty = false; // The data is now clean as it's been refreshed from memory
            // Here you would actually copy the data from the message to the cache line
            // For example: set->lines[i].data = message->data;
            return;
        }
    }

    // If the address is not in the cache, find an invalid line or use LRU policy to replace one
    unsigned long replaceIndex = 0;
    unsigned long maxLRU = 0;
    for (unsigned int i = 0; i < set->maxLines; i++) {
        if (!set->lines[i].valid) {
            replaceIndex = i;
            break;
        } else if (set->lruCounter[i] > maxLRU) {
            replaceIndex = i;
            maxLRU = set->lruCounter[i];
        }
    }

    // Update the line in the cache
    set->lines[replaceIndex].tag = tag;
    set->lines[replaceIndex].valid = true;
    set->lines[replaceIndex].isDirty = false; // The data is now clean as it's been refreshed from memory
    // Here you would actually copy the data from the message to the cache line
    // For example: set->lines[replaceIndex].data = message->data;

    // Update LRU counters for the set
    updateLRUCounter(set, replaceIndex);
}

/**
 * @brief 
 * 
 * @param message 
 * @param allCaches 
 */
void handleInvalidate(message_t *message, cache_t **allCaches) {
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        cache_t *cache = allCaches[i];
        // Convert the address in the message to a cache set index and tag
        unsigned long setIndex = (message->address >> cache->B) & ((1UL << cache->S) - 1);
        unsigned long tag = message->address >> (cache->B + cache->S);
        set_t *set = &cache->setList[setIndex];

        // Invalidate the line if it is present in the cache
        for (unsigned int j = 0; j < set->maxLines; j++) {
            if (set->lines[j].valid && set->lines[j].tag == tag) {
                set->lines[j].valid = false;
                set->lines[j].isDirty = false; // The line is no longer dirty since it's invalidated
                // Send INVALIDATE_ACK if needed by protocol
                break;
            }
        }
    }
}

/**
 * @brief 
 * 
 * @param message 
 * @param interconnect 
 * @param directory 
 */
void handleInvalidateAcknowledge(message_t *message, interconnect_t *interconnect, directory_t *directory) {
    // Lock the directory entry for the given address
    int dirIndex = directoryIndex(message->address);
    pthread_mutex_lock(&directory->lines[dirIndex].lock);

    // Update the directory to reflect that the cache line has been invalidated
    directory_entry_t *dirEntry = &directory->lines[dirIndex];
    dirEntry->existsInCache[message->sourceId] = false;

    // If no other caches have this line, set the directory state to UNCACHED
    bool stillCached = false;
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        if (dirEntry->existsInCache[i]) {
            stillCached = true;
            break;
        }
    }
    if (!stillCached) {
        dirEntry->state = DIR_UNCACHED;
        dirEntry->owner = -1;
    }

    // Unlock the directory entry
    pthread_mutex_unlock(&directory->lines[dirIndex].lock);
}

/**
 * @brief 
 * 
 * @param message 
 * @param interconnect 
 * @param directory 
 */
void handleWriteRequest(message_t *message, interconnect_t *interconnect, directory_t *directory) {
    // Lock the directory entry for the given address
    int dirIndex = directoryIndex(message->address);
    pthread_mutex_lock(&directory->lines[dirIndex].lock);

    directory_entry_t *dirEntry = &directory->lines[dirIndex];

    // Invalidate other caches if the line is cached elsewhere
    if (dirEntry->state != DIR_UNCACHED) {
        for (int i = 0; i < NUM_PROCESSORS; ++i) {
            if (dirEntry->existsInCache[i] && i != message->sourceId) {
                // Construct an invalidate message to the other cache
                message_t invalidateMsg;
                invalidateMsg.type = INVALIDATE;
                invalidateMsg.address = message->address;
                invalidateMsg.sourceId = DIRECTORY_ID; // Assuming DIRECTORY_ID is predefined
                invalidateMsg.destId = i;

                // Send the invalidate message
                interconnectSendMessage(interconnect, invalidateMsg);
            }
        }
    }

    // Update the directory to reflect the new state
    dirEntry->state = DIR_EXCLUSIVE_MODIFIED;
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        dirEntry->existsInCache[i] = false;
    }
    dirEntry->existsInCache[message->sourceId] = true;
    dirEntry->owner = message->sourceId;

    // Unlock the directory entry
    pthread_mutex_unlock(&directory->lines[dirIndex].lock);

    // Update the memory with the new data
    // Assuming there's a function to update the main memory
    // updateMemory(message->address, message->data);

    // Send WRITE_ACKNOWLEDGE to the requesting cache
    message_t writeAckMsg;
    writeAckMsg.type = WRITE_ACKNOWLEDGE;
    writeAckMsg.address = message->address;
    writeAckMsg.sourceId = DIRECTORY_ID; // Assuming DIRECTORY_ID is predefined
    writeAckMsg.destId = message->sourceId;

    // Send the write acknowledge message
    interconnectSendMessage(interconnect, writeAckMsg);
}


/**
 * @brief 
 * 
 * @param message 
 * @param cache 
 */
void handleWriteUpdate(message_t *message, cache_t *cache) {
    // Convert the address in the message to a cache set index and tag
    unsigned long setIndex = (message->address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = message->address >> (cache->B + cache->S);

    // Find the set in the cache where this address should be
    set_t *set = &cache->setList[setIndex];

    // Look for the cache line that matches the address
    for (unsigned int i = 0; i < set->maxLines; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            // Update the line's data with the new data
            // Here you would actually copy the data from the message to the cache line
            // For example: set->lines[i].data = message->data;
            set->lines[i].isDirty = true; // The line is now dirty as it has been updated
            return;
        }
    }

    // If the line is not found in the cache, it means it was evicted,
    // and we don't need to do anything.
}

/**
 * @brief 
 * 
 * @param message 
 * @param cache 
 */
void handleWriteAcknowledge(message_t *message, cache_t *cache) {
    // Convert the address in the message to a cache set index and tag
    unsigned long setIndex = (message->address >> cache->B) & ((1UL << cache->S) - 1);
    unsigned long tag = message->address >> (cache->B + cache->S);

    // Find the set in the cache where this address should be
    set_t *set = &cache->setList[setIndex];

    // Look for the cache line that matches the address
    for (unsigned int i = 0; i < set->maxLines; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            // Acknowledge that the write has been completed
            // Here you might want to change the state of the line, or simply mark it as not dirty
            set->lines[i].isDirty = false;
            // If there's a state associated with the cache line, we might want to update it
            // set->lines[i].state = MODIFIED; // For example
            return;
        }
    }

    // If the line is not found in the cache, it was probably evicted, and we have nothing to do
}

/**
 * @brief 
 * 
 * @param message 
 * @param allCaches 
 */
void handleUpdate(message_t *message, cache_t **allCaches) {
    // This message is intended to update all caches except the one that initiated the write
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        if (allCaches[i]->processor_id != message->sourceId) {
            // Convert the address in the message to a cache set index and tag
            unsigned long setIndex = (message->address >> allCaches[i]->B) & ((1UL << allCaches[i]->S) - 1);
            unsigned long tag = message->address >> (allCaches[i]->B + allCaches[i]->S);

            // Find the set in the cache where this address should be
            set_t *set = &allCaches[i]->setList[setIndex];

            // Look for the cache line that matches the address
            for (unsigned int j = 0; j < set->maxLines; j++) {
                if (set->lines[j].valid && set->lines[j].tag == tag) {
                    // Update the line's data with the new data
                    // Here you would actually copy the data from the message to the cache line
                    // For example: set->lines[j].data = message->data;
                    set->lines[j].isDirty = true; // The line is now dirty as it has been updated
                    // If there's a state associated with the cache line, we might want to update it
                    // set->lines[j].state = SHARED; // For example
                    break;
                }
            }
        }
    }
}


/**
 * @brief 
 * 
 * @param line 
 * @param interconnect 
 */
void serviceInstruction(char *line, interconnect_t *interconnect) {
    int processorId;
    char operation;
    unsigned long address;

    // Parse the line
    if (sscanf(line, "%d %c %lx", &processorId, &operation, &address) != 3) {
        fprintf(stderr, "Error parsing instruction: %s\n", line);
        return;
    }

    // Check if processor ID is valid
    if (processorId < 0 || processorId >= NUM_PROCESSORS) {
        fprintf(stderr, "Invalid processor ID: %d\n", processorId);
        return;
    }

    // Create a message for the instruction
    message_t message;
    message.sourceId = processorId;
    message.destId = processorId; // Assuming the message is targeted at the cache associated with the processor
    message.address = address;

    // Determine the message type based on the operation
    if (operation == 'R') {
        message.type = READ_REQUEST;
    } else if (operation == 'W') {
        message.type = WRITE_REQUEST;
    } else {
        fprintf(stderr, "Invalid operation: %c\n", operation);
        return;
    }

    // Send the message via the interconnect
    interconnectSendMessage(interconnect, message);
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

    // Create threads for each cache
    pthread_t cacheThreads[NUM_PROCESSORS];
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        if (pthread_create(&cacheThreads[i], NULL, cacheThreadFunction, (void *)caches[i])) {
            fprintf(stderr, "Error creating thread for cache %d\n", i);
            return 1;
        }
    }

    // Create thread for the central directory
    pthread_t directoryThread;
    if (pthread_create(&directoryThread, NULL, directoryThreadFunction, (void *)directory)) {
        fprintf(stderr, "Error creating thread for directory\n");
        return 1;
    }

    // Open the trace file
    FILE *traceFile = fopen(argv[1], "r");
    if (traceFile == NULL) {
        perror("Error opening trace file");
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), traceFile)) {
        // Process each line in the trace file
        serviceInstruction(line, interconnect);
    }

    fclose(traceFile);

    // Cleanup: Join all threads and cleanup resources
    for (int i = 0; i < NUM_PROCESSORS; ++i) {
        pthread_join(cacheThreads[i], NULL);
    }
    pthread_join(directoryThread, NULL);

    cleanupSystem();
    return 0;
}
