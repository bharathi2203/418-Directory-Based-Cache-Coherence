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
    interconnect->outgoingQueue = createQueue();
    if (!interconnect->incomingQueue || !interconnect->outgoingQueue) {
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
static message_t createMessage(message_type type, int srcId, int destId, int address) {
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
void processMessageQueue(interconnect_t* interconnect) {
    // Process incoming messages
    while (!isQueueEmpty(interconnect->incomingQueue)) {
        message_t msg = dequeue(interconnect->incomingQueue);

        switch (msg.type) {
            case READ_REQUEST:
                // Handle read requests
                handleReadRequest(interconnect, msg);
                break;
            case WRITE_REQUEST:
                // Handle write requests
                handleWriteRequest(interconnect, msg);
                break;
            case INVALIDATE:
                // Handle invalidate requests
                handleInvalidateRequest(interconnect, msg);
                break;
            case READ_ACKNOWLEDGE:
                handleReadAcknowledge(interconnect, msg);
                break;
            case INVALIDATE_ACK:
                handleInvalidateAcknowledge(interconnect, msg);
                break;
            case WRITE_ACKNOWLEDGE:
                handleWriteAcknowledge(interconnect, msg);
                break;
            default:
                break;
        }
    }

    // Process outgoing messages
    while (!isQueueEmpty(interconnect->outgoingQueue)) {
        message_t msg = dequeue(interconnect->outgoingQueue);

        node_t* node = &interconnect->nodeList[nodeId];
        cache_t* cache = node->cache;
        directory_t* dir = node->directory;

        switch (msg.type) {
            case READ_REQUEST:
                // Process read request - possibly using readFromCache or similar function
                readFromCache(cache, msg.address);
                break;
            case WRITE_REQUEST:
                // Process write request - possibly using writeToCache or similar function
                writeToCache(cache, msg.address);
                break;
            case INVALIDATE:
                // Invalidate the cache line
                invalidateCacheLine(cache, msg.address);
                break;
            case READ_ACKNOWLEDGE:
                // Update cache line state to SHARED
                updateCacheLineState(cache, msg.address, SHARED);
                break;
            case INVALIDATE_ACK:
                // Update directory state
                // Assume function updateDirectoryState exists
                updateDirectoryState(dir, msg.address, DIR_UNCACHED);
                break;
            case WRITE_ACKNOWLEDGE:
                // Update cache line state to MODIFIED
                updateCacheLineState(cache, msg.address, MODIFIED);
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
void handleReadRequest(interconnect_t* interconnect, message_t msg) {
    directory_t* dir = interconnect->nodeList[msg.destId].directory;
    int index = directoryIndex(msg.address);
    directory_entry_t* entry = &dir->lines[index];

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
}

/**
 * @brief Handles write requests by updating directory entries to reflect the exclusive 
 * modification status and sending invalidation requests to other caches as needed.
 * 
 * @param interconnect 
 * @param msg 
 */
void handleWriteRequest(interconnect_t* interconnect, message_t msg) {
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
                sendInvalidate(interconnect, i, msg.address);
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
void handleInvalidateRequest(interconnect_t* interconnect, message_t msg) {
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
    enqueue(interconnect->outgoingQueue, invalidateAckMsg);
}

/**
 * @brief Updates a cache's state with data received in response to a read 
 * request, marking the cache line as shared.
 * 
 * @param interconnect 
 * @param msg 
 */
void handleReadAcknowledge(interconnect_t* interconnect, message_t msg) {
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
void handleInvalidateAcknowledge(interconnect_t* interconnect, message_t msg) {
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
void handleWriteAcknowledge(interconnect_t* interconnect, message_t msg) {
    // Update cache line state to MODIFIED
    cache_t* cache = interconnect->nodeList[msg.sourceId].cache;
    // Check if the cache line exists and then update its state to MODIFIED
    if (findLineInSet(cache->setList[directoryIndex(msg.address)], msg.address)) {
        updateCacheLineState(cache, msg.address, MODIFIED);
    }
}


/**
 * @brief Safely deallocates memory and resources associated with the 
 * interconnect structure, including queues and cache nodes.
 * 
 * @param interconnect 
 */
void freeInterconnect(interconnect_t *interconnect) {
    if (interconnect) {
        if (interconnect->incomingQueue) {
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