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