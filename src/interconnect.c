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
    while (!isQueueEmpty(interconnect->incomingQueue)) {
        message_t msg = *(message_t *)dequeue(interconnect->incomingQueue);
        switch (msg.type) {
            case READ_REQUEST:
                // Handle read requests
                printf("READ_REQUEST\n");
                handleReadRequest(msg);
                break;
            case WRITE_REQUEST:
                // Handle write requests
                handleWriteRequest(msg);
                break;
            case INVALIDATE:
                // Handle invalidate requests
                handleInvalidateRequest(msg);
                break;
            case READ_ACKNOWLEDGE:
                handleReadAcknowledge(msg);
                break;
            case INVALIDATE_ACK:
                handleInvalidateAcknowledge(msg);
                break;
            case WRITE_ACKNOWLEDGE:
                handleWriteAcknowledge(msg);
                break;
            default:
                break;
        }
    }

    // Process outgoing messages
    while (!isQueueEmpty(interconnect->outgoingQueue)) {
        message_t *msg = (message_t *)dequeue(interconnect->outgoingQueue);

        node_t* node = &interconnect->nodeList[msg->destId];
        cache_t* cache = node->cache;
        directory_t* dir = node->directory;

        switch (msg->type) {
            case READ_REQUEST:
                // Process read request - possibly using readFromCache or similar function
                readFromCache(cache, msg->address, msg->sourceId);
                break;
            case WRITE_REQUEST:
                // Process write request - possibly using writeToCache or similar function
                writeToCache(cache, msg->address, msg->sourceId);
                break;
            case INVALIDATE:
                // Invalidate the cache line
                invalidateCacheLine(cache, msg->address);
                break;
            case READ_ACKNOWLEDGE:
                // Update cache line state to SHARED
                updateCacheLineState(cache, msg->address, SHARED);
                break;
            case INVALIDATE_ACK:
                // Update directory state
                updateDirectoryState(dir, msg->address, DIR_UNCACHED);
                break;
            case WRITE_ACKNOWLEDGE:
                // Update cache line state to MODIFIED
                updateCacheLineState(cache, msg->address, MODIFIED);
                break;
            default:
                break;
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
    printf("msg.sourceId: %d, msg.destId: %d, msg.address: %lu", msg.sourceId, msg.destId, msg.address);
    if(msg.destId == msg.sourceId) {
        cache_t* cache = interconnect->nodeList[msg.destId].cache;
        printf("address %lu on this node %d\n", msg.address, msg.destId);
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
        set_t *set = &cache->setList[index];
        line_t *line = findLineInSet(*set, address);

        if (line != NULL && line->valid && line->tag == address) {
            // Cache hit
            printf("Cache HIT: Processor %d reading address %d\n", cache->processor_id, address);
            cache->hitCount++;
            updateLineUsage(line);  // Update line usage on hit
        } else {
            // Cache miss
            printf("Cache MISS: Processor %d reading address %d\n", cache->processor_id, address);
            cache->missCount++;
            // fetchFromDirectory(interconnect->nodeList[cache->processor_id].directory, address, cache->processor_id);
            addLineToCacheSet(cache, set, address, EXCLUSIVE);
        }

    }
    else {
        message_t Readmsg = {
            .type = READ_REQUEST,
            .sourceId = msg.sourceId,
            .destId = msg.destId,
            .address = msg.address
        };

        // Enqueue the message to the outgoing queue
        enqueue(interconnect->outgoingQueue, &Readmsg);
    }
}

/**
 * @brief Handles write requests by updating directory entries to reflect the exclusive 
 * modification status and sending invalidation requests to other caches as needed.
 * 
 * @param interconnect 
 * @param msg 
 */
void handleWriteRequest(message_t msg) {
    // Locate the directory for the destination node
    directory_t* dir = interconnect->nodeList[msg.destId].directory;
    int index = directoryIndex(msg.address);
    directory_entry_t* entry = &dir->lines[index];

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
    enqueue(interconnect->outgoingQueue, &invalidateAckMsg);
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
    cache_t* cache = interconnect->nodeList[msg.sourceId].cache;
    // Check if the cache line exists and then update its state to MODIFIED
    if (findLineInSet(cache->setList[directoryIndex(msg.address)], msg.address)) {
        updateCacheLineState(cache, msg.address, MODIFIED);
    }
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
    int index = directoryIndex(address);
    set_t *set = &cache->setList[index];
    line_t *line = findLineInSet(*set, address);

    if (line != NULL && line->valid && line->tag == address) {
        // Cache hit
        printf("Cache HIT: Processor %d reading address %d\n", cache->processor_id, address);
        cache->hitCount++;
        updateLineUsage(line);  // Update line usage on hit
    } else {
        // Cache miss
        printf("Cache MISS: Processor %d reading address %d\n", cache->processor_id, address);
        cache->missCount++;
        
        printf("bringing line into cache\n");
        addLineToCacheSet(cache, set, address, SHARED);
    }
    fetchFromDirectory(interconnect->nodeList[cache->processor_id].directory, address, srcID);
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
    set_t *set = &cache->setList[index];
    line_t *line = findLineInSet(*set, address);

    if (line != NULL && line->valid && calculateTag(address, main_S, main_B) == address) {
        // Cache hit
        printf("Cache HIT: Processor %d writing to address %d\n", cache->processor_id, address);
        cache->hitCount++;
        line->state = MODIFIED;
        line->isDirty = true;
        updateLineUsage(line);  // Update line usage on hit
    } else {
        // Cache miss
        printf("Cache MISS: Processor %d writing to address %d\n", cache->processor_id, address);
        cache->missCount++;
        addLineToCacheSet(cache, set, address, MODIFIED);
    }
    // Notify the directory that this cache now has a modified version of the line
    fetchFromDirectory(interconnect->nodeList[cache->processor_id].directory, address, srcId);
    updateDirectory(interconnect->nodeList[cache->processor_id].directory, address, srcId, DIR_EXCLUSIVE_MODIFIED);
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
 */
void fetchFromDirectory(directory_t* directory, unsigned long address, int requestingProcessorId) {
    int index = directoryIndex(address);
    directory_entry_t* line = &directory->lines[index];
    int nodeID = address / NUM_LINES;

    // Check the state of the directory line
    if (line->state == DIR_EXCLUSIVE_MODIFIED) {
        // If the line is exclusively modified, we need to fetch it from the owning cache
        int owner = line->owner;
        if (owner != -1) {
            // Invalidate the line in the owner's cache
            sendInvalidate(nodeID, owner, address);

            // Simulate transferring the line to the requesting cache
            sendFetch(nodeID, requestingProcessorId, address);
        }
    } else if (line->state == DIR_UNCACHED || line->state == DIR_SHARED) {
        // If the line is uncached or shared, fetch it from the memory
        printf("fetching from another directory, line is shared\n");
        sendReadData(nodeID, requestingProcessorId, address, (line->state == DIR_UNCACHED));
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
void sendReadData(int srcId, int destId, unsigned long address, bool exclusive) {
    // Create a READ_ACKNOWLEDGE message
    message_t readDataMsg = {
        .type = READ_ACKNOWLEDGE,
        .sourceId = srcId, // -1 or a specific ID if the directory has an ID
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
void sendFetch(int srcId, int destId, unsigned long address) {
    // Create a FETCH message
    message_t fetchMsg = {
        .type = FETCH,
        .sourceId = srcId,
        .destId = destId,
        .address = address
    };

    // Enqueue the message to the outgoing queue
    enqueue(interconnect->outgoingQueue, &fetchMsg);
}
