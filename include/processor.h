/**
 * @file processor.h
 * @brief 
 */

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "interconnect.h"
#include "single_cache.h"

#define NUM_PROCESSORS 4  
#define NUM_LINES 256

// States for a cache line for a directory based approach 
typedef struct processor {
    int processor_id;
    interconnect_t* interconnect;
    cache_t* cache; 
    directory_t* directory; // central directory struct for now; how can we make this OO so limited pointer, central, and sparse are subclasses of directory? 
} processor_t;

void processMessage(processor_t* processor, message_t* message); 

#endif // PROCESSOR_H