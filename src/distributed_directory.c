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
int timer = 0;
interconnect_stats_t *interconnect_stats = NULL;

/**
 * @brief Parse a line from the trace file and perform the corresponding operation
 * 
 * @param line 
 * @param interconnect 
 */
void processTraceLine(int processorId, char operation, unsigned long address) {

    message_t msg;
    // Convert the address to the local node's index, assuming the address includes the node information
    int localNodeIndex = address / (NUM_LINES * (1 << main_B));
    switch (operation) {
        case 'R':
            // Send a read request to the interconnect queue
            msg.type = READ_REQUEST;
            msg.sourceId = processorId;
            msg.destId = localNodeIndex;
            msg.address = address;
            enqueue(interconnect->incomingQueue, msg.type, msg.sourceId, msg.destId, msg.address);
            processMessageQueue();
            break;
        case 'W':
            // Send a write request to the interconnect queue
            msg.type = WRITE_REQUEST;
            msg.sourceId = processorId;
            msg.destId = localNodeIndex;
            msg.address = address;
            enqueue(interconnect->incomingQueue, msg.type, msg.sourceId, msg.destId, msg.address);
            processMessageQueue();
            break;
        default:
            fprintf(stderr, "Unknown operation '%c' in trace file\n", operation);
            break;
    }
}

void print_interconnect_stats() {
    printf("\nInterconnect_stats: \n");
    printf("\ninterconnect_stats->totalMemReads: %d", interconnect_stats->totalMemReads);
    printf("\ninterconnect_stats->totalReadRequests: %d", interconnect_stats->totalReadRequests);
    printf("\ninterconnect_stats->totalWriteRequests: %d", interconnect_stats->totalWriteRequests);
    printf("\ninterconnect_stats->totalInvalidations: %d", interconnect_stats->totalInvalidations);
    printf("\ninterconnect_stats->totalStateUpdates: %d", interconnect_stats->totalStateUpdates);
    printf("\ninterconnect_stats->totalReadAcks: %d", interconnect_stats->totalReadAcks);
    printf("\ninterconnect_stats->totalWriteAcks: %d ", interconnect_stats->totalWriteAcks);
    printf("\ninterconnect_stats->totalFetchRequests: %d", interconnect_stats->totalFetchRequests);
} 

/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[]) {
    printf("in main");
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tracefile>\n", argv[0]);
        return 1;
    }

    // Initialize the system
    interconnect = createInterconnect(NUM_LINES, main_S, main_E, main_B);

    // Initialize stats struct 
    interconnect_stats = malloc(sizeof(interconnect_stats_t));
    interconnect_stats->totalMemReads = 0;
    interconnect_stats->totalReadRequests = 0;
    interconnect_stats->totalWriteRequests = 0;
    interconnect_stats->totalInvalidations = 0;
    interconnect_stats->totalStateUpdates = 0;
    interconnect_stats->totalReadAcks = 0;
    interconnect_stats->totalWriteAcks = 0;
    interconnect_stats->totalFetchRequests = 0;

    timer = 0;

    // Open the trace file
    FILE *traceFile = fopen(argv[1], "r");
    if (traceFile == NULL) {
        perror("Error opening trace file");
        return 1;
    }

    // Process each line in the trace file
    int procId = 0;
    unsigned long address = 0;
    char instr = '\0';

    while (fscanf(traceFile, "%d %c %lx\n", &procId, &instr, &address) != EOF) {
        printf("%d %c %lx\n", procId, instr, address);
        processTraceLine(procId, instr, address);  
        // print directories
        
    }

    for(int i = 0; i < NUM_PROCESSORS; i++) {
            printf("\nnode %d\n", i);
            directory_t* dir = interconnect->nodeList[i].directory;
            for(int j = 0; j < NUM_LINES; j++) {
                directory_entry_t entry = dir->lines[j];
                if(entry.state != DIR_UNCACHED) {
                    printf("dir line: %d state: %d, owner: %d\n", j, entry.state, entry.owner); 
                    printf("exists in cache: ");
                    for(int k = 0; k < LIM_PTR_DIR_ENTRIES; k++) {
                        printf("%d ", entry.existsInCache[k]);
                    }
                    printf("\n");
                }
            }
            printf("\n \nprocessor id: %d\n", i);
            printCache(interconnect->nodeList[i].cache);
            makeSummary(interconnect->nodeList[i].cache);
    }
    print_interconnect_stats();
    // Cleanup and close the file
    fclose(traceFile);

    // Cleanup resources
    // printf("interconnect in main %p\n", (void*)interconnect);
    // printf("incomingQueue in main %p\n", (void*)interconnect->incomingQueue);
    freeInterconnect();

    return 0;
}