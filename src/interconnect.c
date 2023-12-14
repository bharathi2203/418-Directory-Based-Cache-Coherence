/**
 * @file interconnect.c
 * @brief 
 */
#include <interconnect.h>


/**
 * @brief Create a Interconnect object
 * Create and Initializes the interconnect system with specified parameters.
 * 
 * @param numLines 
 * @param s 
 * @param e 
 * @param b 
 * @return interconnect_t* 
 */
interconnect_t *createInterconnect(int numLines, int s, int e, int b) {
    interconnect_t *interconnect = (interconnect_t *)malloc(sizeof(interconnect_t));
    if (interconnect == NULL) {
        return NULL;
    }

    interconnect->incomingQueue = createQueue();
    // printf("createInterconnect %p\n", (void*)interconnect->incomingQueue);
    interconnect->outgoingQueue = createQueue();
    if (!interconnect->incomingQueue || !interconnect->outgoingQueue) {
        // printf("freeing queue in create\n");
        freeQueue(interconnect->incomingQueue);
        freeQueue(interconnect->outgoingQueue);
        free(interconnect);
        return NULL;
    }

    for (int i = 0; i < NUM_PROCESSORS; i++) {
        interconnect->nodeList[i].directory = initializeDirectory(numLines);
        interconnect->nodeList[i].cache = initializeCache(s, e, b, i);
        if (!interconnect->nodeList[i].directory || !interconnect->nodeList[i].cache) {
            // Clean up in case of failure
            for (int j = 0; j < i; j++) {
                freeDirectory(interconnect->nodeList[j].directory);
                freeCache(interconnect->nodeList[j].cache);
            }
            // printf("freeing queue in create\n");
            freeQueue(interconnect->incomingQueue);
            freeQueue(interconnect->outgoingQueue);
            free(interconnect);
            return NULL;
        }
    }

    return interconnect;
}


/**
 * @brief Create a Message object
 * 
 * @param type 
 * @param srcId 
 * @param destId 
 * @param address 
 * @return message_t 
 */
message_t createMessage(message_type type, int srcId, int destId, unsigned long address) {
    message_t msg;
    msg.type = type;
    msg.sourceId = srcId;
    msg.destId = destId;
    msg.address = address;
    return msg;
}

/**
 * @brief Process interconnect input and output queues 
 * 
 * @param interconnect 
 */
void processMessageQueue() {
    // Process incoming messages
    while(!isQueueEmpty(interconnect->incomingQueue) || !isQueueEmpty(interconnect->outgoingQueue)){

        while (!isQueueEmpty(interconnect->incomingQueue)) {
            timer += 1;
            //printf("incoming queue");
            // print queue
            // printQueue(interconnect->incomingQueue);
            // printQueue(interconnect->outgoingQueue);
            message_t *msg = dequeue(interconnect->incomingQueue);

            switch (msg->type) {
                case READ_REQUEST:
                    // Handle read requests
                    // printf("READ_REQUEST\n");
                    handleReadRequest(*msg);
                    interconnect_stats->totalReadRequests += 1;
                    break;
                case WRITE_REQUEST:
                    // Handle write requests
                    handleWriteRequest(*msg);
                    interconnect_stats->totalWriteRequests += 1;
                    break;
                case INVALIDATE:
                    // Handle invalidate requests
                    handleInvalidateRequest(*msg);
                    printf("\n invalidate %lx\n", msg->address);
                    interconnect_stats->totalInvalidations += 1;
                    break;
                case READ_ACKNOWLEDGE:
                    handleReadAcknowledge(*msg);
                    interconnect_stats->totalReadAcks += 1;
                    break;
                case INVALIDATE_ACK:
                    handleInvalidateAcknowledge(*msg);
                    break;
                // case WRITE_ACKNOWLEDGE:
                //     handleWriteAcknowledge(*msg);
                //     break;
                default:
                    break;
            }
            free(msg);
        }

        // Process outgoing messages
        while (!isQueueEmpty(interconnect->outgoingQueue)) {
            timer += 1;
            // printf("outgoing queue");
            // print queue
            // printQueue(interconnect->incomingQueue);
            // printQueue(interconnect->outgoingQueue);
            message_t *msg = (message_t *)dequeue(interconnect->outgoingQueue);

            node_t* node = &interconnect->nodeList[msg->destId];
            // printf("outgoing queue destId: %d\n", msg->destId);
            cache_t* cache = node->cache;

            directory_t* dir = node->directory;

            switch (msg->type) {
                case READ_REQUEST:
                    // Process read request - possibly using readFromCache or similar function
                    readFromCache(cache, msg->address, msg->sourceId);
                    interconnect_stats->totalReadRequests += 1;
                    break;
                case WRITE_REQUEST:
                    // Process write request - possibly using writeToCache or similar function
                    // printf("writeToCache srcId: %d\n", msg->sourceId);
                    writeToCache(cache, msg->address, msg->sourceId);
                    interconnect_stats->totalWriteRequests += 1;
                    break;
                case INVALIDATE:
                    // Invalidate the cache line
                    invalidateCacheLine(cache, msg->address);
                    printf("\n invalidate %lx\n", msg->address);
                    interconnect_stats->totalInvalidations += 1;
                    break;
                case READ_ACKNOWLEDGE:
                    // Update cache line state to SHARED
                    updateCacheLineState(cache, msg->address, SHARED);
                    interconnect_stats->totalReadAcks += 1;
                    break;
                case INVALIDATE_ACK:
                    // Update directory state
                    updateDirectoryState(dir, msg->address, DIR_UNCACHED);
                    break;
                case WRITE_ACKNOWLEDGE:
                    // Update cache line state to MODIFIED
                    updateCacheLineState(cache, msg->address, MODIFIED);
                    interconnect_stats->totalWriteAcks += 1;
                    break;
                case FETCH:
                    updateCacheLineState(cache, msg->address, SHARED);
                    interconnect_stats->totalFetchRequests += 1;
                    break;
                default:
                    break;
            }
            free(msg);
        }
    }
}

/**
 * @brief  Manages read requests in a distributed cache system by updating the directory 
 * state and ownership of cache lines based on the current status of the requested data.
 * 
 * @param interconnect 
 * @param msg 
 */
void handleReadRequest(message_t msg) {
    directory_t* dir = interconnect->nodeList[msg.destId].directory;
    int index = directoryIndex(msg.address);
    directory_entry_t* entry = &dir->lines[index];
    // printf("msg.sourceId: %d, msg.destId: %d, msg.address: %lu\n", msg.sourceId, msg.destId, msg.address);
    if(msg.destId == msg.sourceId) {
        cache_t* cache = interconnect->nodeList[msg.destId].cache;
        // printf("address %lu on this node %d\n", msg.address, msg.destId);
        // If the line is uncached or already shared, simply update presence bits
        if (entry->state == DIR_UNCACHED || entry->state == DIR_SHARED) {
            entry->state = DIR_SHARED;
            entry->existsInCache[msg.sourceId] = true;

        } else if (entry->state == DIR_EXCLUSIVE_MODIFIED) {
            // The line is currently exclusively modified in one cache.
            // We need to downgrade it to shared and send it to the requesting cache.        
            // sendDowngrade(interconnect, entry->owner, msg.address);

            // Update the directory to shared state
            entry->state = DIR_SHARED;
            entry->existsInCache[entry->owner] = true; // The owner still has it in shared state now
            entry->existsInCache[msg.sourceId] = true; // The requester will have it in shared state
            entry->owner = -1; // No single owner anymore
        }


        // Cache Updates 
        unsigned long address = msg.address;
        int index = directoryIndex(address);
        int setIndex = calculateSetIndex(address, cache->S, cache->B);
        set_t *set = &cache->setList[setIndex];

        // printf("msg.sourceId: %d, msg.destId: %d, msg.address: %lu\n", msg.sourceId, msg.destId, msg.address);
        // printCache(cache);
        line_t *line = findLineInSet(*set, calculateTag(address, cache->S, cache->B));
        // printf("msg.sourceId: %d, msg.destId: %d, msg.address: %lu\n", msg.sourceId, msg.destId, msg.address);

        if (line != NULL && line->valid && line->tag ==  calculateTag(address, cache->S, cache->B)) {
            // Cache hit
            // printf("Cache HIT: Processor %d reading address %lu\n", cache->processor_id, address);
            cache->hitCount++;
            updateLineUsage(line);  // Update line usage on hit
        } else {
            // Cache miss
            // printf("here\n");
            // printf("Cache MISS: Processor %d reading address %lu\n", cache->processor_id, address);
            cache->missCount++;
            // fetchFromDirectory(interconnect->nodeList[cache->processor_id].directory, address, cache->processor_id);
            addLineToCacheSet(cache, set, address, EXCLUSIVE);
        }

    }
    else {
        message_t Readmsg = {
            .type = READ_REQUEST,
            .sourceId = msg.destId,
            .destId = msg.sourceId,
            .address = msg.address
        };

        // Enqueue the message to the outgoing queue
        enqueue(interconnect->outgoingQueue, Readmsg.type, Readmsg.sourceId, Readmsg.destId, Readmsg.address);
    }
}

/**
 * @brief Handles write requests by updating directory entries to reflect the exclusive 
 * modification status and sending invalidation requests to other caches as needed.
 * 
 * @param interconnect 
 * @param msg 
 */
 // TODO: what happens if 
void handleWriteRequest(message_t msg) {
    // Locate the directory for the destination node
    directory_t* dir = interconnect->nodeList[msg.destId].directory;
    int index = directoryIndex(msg.address);
    directory_entry_t* entry = &dir->lines[index];

    if(msg.destId == msg.sourceId) { 
        // Update the directory entry for the write request
        if (entry->state != DIR_UNCACHED) {
            // Invalidate other caches if necessary
            for (int i = 0; i < NUM_PROCESSORS; i++) {
                if (i != msg.sourceId && entry->existsInCache[i]) {
                    // Invalidate other caches
                    sendInvalidate(msg.sourceId, i, msg.address);
                    entry->existsInCache[i] = false;
                }
            }
        }
        // Update the directory to exclusive modified state
        entry->state = DIR_EXCLUSIVE_MODIFIED;
        entry->owner = msg.sourceId;
        entry->existsInCache[msg.sourceId] = true;

        // Cache Updates 
        cache_t *cache = interconnect->nodeList[msg.sourceId].cache;
        int setIndex = calculateSetIndex(msg.address, cache->S, cache->B);
        line_t *line = findLineInSet(cache->setList[setIndex], calculateTag(msg.address, cache->S, cache->B));

        if (line != NULL && line->valid && line->tag ==  calculateTag(msg.address, cache->S, cache->B)) {
            // Cache hit
            // printf("Cache HIT: Processor %d reading address %lu\n", cache->processor_id, msg.address);
            cache->hitCount++;
            updateLineUsage(line);  // Update line usage on hit
            line->state = MODIFIED;
            line->valid = true;
            line->isDirty = true;
        } else {
            // Cache miss
            // printf("Cache MISS: Processor %d reading address %lu\n", cache->processor_id, msg.address);
            cache->missCount++;
            // fetchFromDirectory(interconnect->nodeList[cache->processor_id].directory, address, cache->processor_id);
            addLineToCacheSet(cache, &cache->setList[setIndex], msg.address, MODIFIED);
        }
        // because directory says it's uncached, we don't need to do anything to any other caches?        

    }
    else {
        message_t writeMsg = {
            .type = WRITE_REQUEST,
            .sourceId = msg.destId,
            .destId = msg.sourceId, 
            .address = msg.address
        };

        // Enqueue the message to the outgoing queue
        enqueue(interconnect->outgoingQueue, writeMsg.type, writeMsg.sourceId, writeMsg.destId, writeMsg.address);

    }
    
}

/**
 * @brief Processes invalidate requests by invalidating the specified cache line 
 * and sending an acknowledgment back to the requesting component.
 * 
 * @param interconnect 
 * @param msg 
 */
void handleInvalidateRequest(message_t msg) {
    // Invalidate the cache line in the specified cache
    cache_t* cache = interconnect->nodeList[msg.destId].cache;
    invalidateCacheLine(cache, msg.address);

    // Create an INVALIDATE_ACK message
    message_t invalidateAckMsg;
    invalidateAckMsg.type = INVALIDATE_ACK;
    invalidateAckMsg.sourceId = msg.destId; // ID of the component sending the ack
    invalidateAckMsg.destId = msg.sourceId; // ID of the component that initiated the invalidate request
    invalidateAckMsg.address = msg.address; // Address of the invalidated cache line

    // Enqueue the acknowledgment message to the outgoing queue
    enqueue(interconnect->outgoingQueue, invalidateAckMsg.type, invalidateAckMsg.sourceId, 
        invalidateAckMsg.destId, invalidateAckMsg.address);
}

/**
 * @brief Updates a cache's state with data received in response to a read 
 * request, marking the cache line as shared.
 * 
 * @param interconnect 
 * @param msg 
 */
void handleReadAcknowledge(message_t msg) {
    // Update cache with the data received
    cache_t* cache = interconnect->nodeList[msg.destId].cache;
    updateCacheLineState(cache, msg.address, SHARED);
}

/**
 * @brief Updates the directory to indicate a cache line is invalidated 
 * following the receipt of an invalidation acknowledgment.
 * 
 * @param interconnect 
 * @param msg 
 */
void handleInvalidateAcknowledge(message_t msg) {
    // Update directory to reflect the cache line is invalidated
    directory_t* dir = interconnect->nodeList[msg.sourceId].directory;
    updateDirectoryState(dir, msg.address, DIR_UNCACHED);
}

/**
 * @brief Updates the state of a cache line to modified in response 
 * to a write acknowledgment, ensuring data consistency.
 * 
 * @param interconnect 
 * @param msg 
 */
void handleWriteAcknowledge(message_t msg) {
    // Update cache line state to MODIFIED
    /*
    cache_t* cache = interconnect->nodeList[msg.sourceId].cache;
    // Check if the cache line exists and then update its state to MODIFIED
    int setIndex = calculateSetIndex(msg.address, cache->S, cache->B);
    if (findLineInSet(cache->setList[setIndex], calculateTag(msg.address, cache->S, cache->B))) {
        updateCacheLineState(cache, msg.address, MODIFIED);
    }
    */

    
}

/**
 * @brief Free directory_t object. 
 * 
 * @param dir 
 */
void freeDirectory(directory_t* dir) {
    if (dir) {
        if (dir->lines) {
            free(dir->lines);
        }
        free(dir);
    }
}

/**
 * @brief Updates the state of a specific cache line.
 * 
 * @param cache 
 * @param address 
 * @param newState 
 * @return true 
 * @return false 
 */
bool updateCacheLineState(cache_t *cache, unsigned long address, block_state newState) {
    unsigned long setIndex = calculateSetIndex(address, cache->S, cache->B);
    unsigned long tag = calculateTag(address, cache->S, cache->B);
    bool lineFound = false;

    for (unsigned int i = 0; i < cache->E; ++i) {
        line_t *line = &cache->setList[setIndex].lines[i];
        if (line->tag == tag && line->valid) {
            line->state = newState;
            lineFound = true;
            break;
        }
    }

    // Additional logic for directory update

    return lineFound;
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
 * @brief Safely deallocates memory and resources associated with the 
 * interconnect structure, including queues and cache nodes.
 * 
 * @param interconnect 
 */
void freeInterconnect() {
    if (interconnect) {
        // printf("freeInterconnect %p\n", (void*)interconnect->incomingQueue);
        if (interconnect->incomingQueue) {
            printf("%d\n", queueSize(interconnect->incomingQueue));
            freeQueue(interconnect->incomingQueue);
        }
        if (interconnect->outgoingQueue) {
            freeQueue(interconnect->outgoingQueue);
        }
        for (int i = 0; i < NUM_PROCESSORS; i++) {
            freeDirectory(interconnect->nodeList[i].directory);
            freeCache(interconnect->nodeList[i].cache);
        }
        free(interconnect);
    }
}

/**
 * @brief Reads data from a specified address in the cache. 
 *
 * This function attempts to read data from the cache at a given address. 
 * It first checks if the desired line is in the cache (cache hit). If 
 * found and valid, it increments the hit counter and updates the line's 
 * usage for the LRU policy. 
 * 
 * In case of a cache miss, it increments the miss counter and then 
 * fetches the required line from the directory or another cache, 
 * updating the local cache with the shared state of the line. This 
 * function handles both hit and miss scenarios, ensuring proper cache 
 * coherence and consistency.
 * 
 * @param cache         The cache from which data is to be read.
 * @param address       The memory address to read from.
 */
void readFromCache(cache_t *cache, unsigned long address, int srcID) {
    // printf("readFromCache %d\n", srcID);
    int nodeId = address/NUM_LINES;
    int index = directoryIndex(address);
    int setIndex = calculateSetIndex(address, cache->S, cache->B);
    set_t *set = &cache->setList[setIndex];
    line_t *line = findLineInSet(*set, calculateTag(address, cache->S, cache->B));

    if (line != NULL && line->valid && line->tag == calculateTag(address, cache->S, cache->B)) {
        // Cache hit
        // printf("readFromCache Cache HIT: Processor %d reading address %lu\n", cache->processor_id, address);
        cache->hitCount++;
        updateLineUsage(line);  // Update line usage on hit
        line->state = SHARED;
    } else {
        // Cache miss

        if(line !=NULL) {
            // printf("\nline: %p", (void*)line);
        }
        // printf("readFromCache Cache MISS: Processor %d reading address %lu\n", cache->processor_id, address);
        cache->missCount++;
        
        // printf("bringing line into cache\n");
        addLineToCacheSet(cache, set, address, SHARED);
        
    }
    fetchFromDirectory(interconnect->nodeList[srcID].directory, address, cache->processor_id, true);
}

/**
 * @brief Writes data to a specified address in the cache.
 * 
 * This function is responsible for writing data to the cache at a given address. 
 * It checks if the target line is present and valid in the cache (cache hit). If 
 * a hit occurs, it updates the line's state to MODIFIED, marks it as dirty, and 
 * updates its usage for LRU policy. 
 * 
 * In the event of a cache miss, it increments the miss counter, fetches the line 
 * from the directory or another cache, and adds it to the cache set with a 
 * MODIFIED state. This function also notifies the directory of any changes, 
 * ensuring the line is updated to the exclusive modified state in the directory, 
 * maintaining cache coherence.
 * 
 * @param cache             The cache where data is to be written.
 * @param address           The memory address to write to.
 */
void writeToCache(cache_t *cache, unsigned long address, int srcId) {
    int index = directoryIndex(address);
    int setIndex = calculateSetIndex(address, cache->S, cache->B);
    set_t *set = &cache->setList[setIndex];
    line_t *line = findLineInSet(*set, calculateTag(address, cache->S, cache->B));

    if (line != NULL && line->valid && calculateTag(address, main_S, main_B) == line->tag) {
        // Cache hit
        // printf("Cache HIT: Processor %d writing to address %lu\n", cache->processor_id, address);
        cache->hitCount++;
        line->state = MODIFIED;
        line->isDirty = true;
        updateLineUsage(line);  // Update line usage on hit

    } else {
        // Cache miss
        // printf("writeToCache Cache MISS: Processor %d writing to address %lu\n", cache->processor_id, address);
        cache->missCount++;
        addLineToCacheSet(cache, set, address, MODIFIED);
    }
    // Notify the directory that this cache now has a modified version of the line
    fetchFromDirectory(interconnect->nodeList[srcId].directory, address, cache->processor_id, false);
    // void fetchFromDirectory(directory_t* directory, unsigned long address, int requestingProcessorId) {
    // printf("\n update dir: srcID: %d", srcId);
    updateDirectory(interconnect->nodeList[srcId].directory, address, cache->processor_id, DIR_EXCLUSIVE_MODIFIED);
}

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
 * @param read                      if true then read, else write 
 */
void fetchFromDirectory(directory_t* directory, unsigned long address, int requestingProcessorId, bool read) {
    int index = directoryIndex(address);
    directory_entry_t* line = &directory->lines[index];
    int home_node = address / (NUM_LINES * (1 << main_B));

    // Check the state of the directory line
    if (line->state == DIR_EXCLUSIVE_MODIFIED) {
        // If the line is exclusively modified, we need to fetch it from the owning cache
        int owner = line->owner;
        if (owner != -1) {
            // Invalidate the line in the owner's cache
            // printf("\n spot 3: %lu", address);
            // sendInvalidate(nodeID, owner, address); // TODO: why do you invalidate the owner? 

            // Simulate transferring the line to the requesting cache
            sendFetch(requestingProcessorId, owner, address);
            sendFetch(home_node, owner, address); // update home node too 


            // set each cache's state to shared for this line
            cache_t* cache = interconnect->nodeList[owner].cache;
            int setIndex = calculateSetIndex(address, cache->S, cache->B);
            unsigned long tag = calculateTag(address, cache->S, cache->B);
            line_t* ownerCacheLine = findLineInSet(interconnect->nodeList[owner].cache->setList[setIndex], tag);
            ownerCacheLine->state = SHARED;
            cache = interconnect->nodeList[requestingProcessorId].cache;
            line_t* requestorCacheLine = findLineInSet(interconnect->nodeList[requestingProcessorId].cache->setList[setIndex], tag);
            requestorCacheLine->state = (read) ? SHARED : MODIFIED;
            cache = interconnect->nodeList[home_node].cache;
            line_t* homeCacheLine = findLineInSet(interconnect->nodeList[home_node].cache->setList[setIndex], tag);
            if (homeCacheLine != NULL) {
                homeCacheLine->state = SHARED;
            }
            
            // printf("\n home_node: %d, owner: %d, reqProcessorId: %d", home_node, owner, requestingProcessorId);
            

            // update the requestor directory 
            // updateDirectory(directory_t* directory, unsigned long address, int cache_id, directory_state newState)
        }
    } else if (line->state == DIR_UNCACHED || line->state == DIR_SHARED) {
        // If the line is uncached or shared, fetch it from the memory
        // printf("fetching from another directory, line is shared\n");
        // printf("fetchFromDirectory sendReadData\n");
        sendAck(home_node, requestingProcessorId, address, read);
    }

    // Update the directory entry
    line->state = DIR_SHARED;
    line->owner = -1; // TODO: should the owner be the newest sharer? 
    
    line->existsInCache[requestingProcessorId] = true;
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
 * @param write  
 */
void sendAck(int srcId, int destId, unsigned long address, bool read) {
    // Create a READ_ACKNOWLEDGE message
    message_t dataMsg = {
        .type = read ? READ_ACKNOWLEDGE : WRITE_ACKNOWLEDGE,
        .sourceId = srcId, // -1 or a specific ID if the directory has an ID
        .destId = destId,
        .address = address
    };

    // Enqueue the message to the outgoing queue
    enqueue(interconnect->outgoingQueue, dataMsg.type, dataMsg.sourceId, 
        dataMsg.destId, dataMsg.address);
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
void sendFetch(int srcId, int destId, unsigned long address) {
    // Create a FETCH message
    message_t fetchMsg = {
        .type = FETCH,
        .sourceId = srcId,
        .destId = destId,
        .address = address
    };

    // Enqueue the message to the outgoing queue
    enqueue(interconnect->outgoingQueue, fetchMsg.type, fetchMsg.sourceId, 
        fetchMsg.destId, fetchMsg.address);
}
