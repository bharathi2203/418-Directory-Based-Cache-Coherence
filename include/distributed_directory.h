/**
 * @file distributed_directory.h
 * @brief Implement a central directory based cache coherence protocol. 
 */

#ifndef DISTRIBUTED_DIRECTORY_H
#define DISTRIBUTED_DIRECTORY_H

#include <utils.h> 
// Function declarations for directory
void updateLineUsage(line_t *line);
void processTraceLine(char *line, interconnect_t* interconnect);

// Function declarations for benchmarking / testing 
// void displayUsage(void);

#endif // DISTRIBUTED_DIRECTORY_H
