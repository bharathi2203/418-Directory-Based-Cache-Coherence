/**
 * @file distributed_directory.h
 * @brief Implement a central directory based cache coherence protocol. 
 */

#ifndef DISTRIBUTED_DIRECTORY_H
#define DISTRIBUTED_DIRECTORY_H

#include <utils.h> 
// Function declarations for directory
void updateLineUsage(line_t *line);
void processTraceLine(int processorId, char instr, unsigned long address);
void print_interconnect_stats(void); 

// Function declarations for benchmarking / testing 
// void displayUsage(void);

#endif // DISTRIBUTED_DIRECTORY_H
