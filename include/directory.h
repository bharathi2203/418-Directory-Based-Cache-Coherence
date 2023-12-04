/**
 * @file directory.h
 * @brief 
 */

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define NUM_PROCESSORS 4  
#define NUM_LINES 256

// States for a cache line for a directory based approach 
typedef enum {
    DIR_UNCACHED,
    DIR_SHARED,
    DIR_EXCLUSIVE_MODIFIED
} directory_state;

#endif // DIRECTORY_H