/**
 * @file single_cache.h
 * @brief Implement basic LRU cache. 
 */

#ifndef CACHE_H
#define CACHE_H

#include <utils.h>

// Function declarations
int findCacheWithLine(unsigned long address);
void processReadRequest(cache_t *cache, int sourceId, unsigned long address);
void processWriteRequest(cache_t *cache, int sourceId, unsigned long address);
void sendReadResponse(interconnect_t *interconnect, int destId, unsigned long address);
void notifyStateChangeToShared(interconnect_t *interconnect, int cacheId, unsigned long address);
void sendWriteAcknowledge(interconnect_t *interconnect, int destId, unsigned long address);
void fetchDataFromDirectoryOrCache(cache_t *cache, unsigned long address);
void printCache(cache_t *cache);



#endif /* CACHE_H */
