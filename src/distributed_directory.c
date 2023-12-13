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


/**
 * @brief Parse a line from the trace file and perform the corresponding operation
 * 
 * @param line 
 * @param interconnect 
 */
void processTraceLine(int processorId, char operation, unsigned long address) {

    message_t msg;
    // Convert the address to the local node's index, assuming the address includes the node information
    int localNodeIndex = address / NUM_LINES;
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

    while (fscanf(traceFile, "%d %c %ld\n", &procId, &instr, &address) != EOF) {
        printf("%d %c %ld\n", procId, instr, address);
        processTraceLine(procId, instr, address);  
        // print directories
        for(int i = 0; i < NUM_PROCESSORS; i++) {
            printf("\nnode %d\n", i);
            directory_t* dir = interconnect->nodeList[i].directory;
            for(int j = 0; j < NUM_LINES; j++) {
                directory_entry_t entry = dir->lines[j];
                if(entry.state != DIR_UNCACHED) {
                    printf("dir line: %d state: %d, owner: %d\n", j, entry.state, entry.owner); 
                    printf("exists in cache: ");
                    for(int k = 0; k < NUM_PROCESSORS; k++) {
                        printf("%d: %d ", k, entry.existsInCache[k]);
                    }
                    printf("\n");
                }
            }
            printf("\n \nprocessor id: %d\n", i);
            printCache(interconnect->nodeList[i].cache);
        }
    }

    // Cleanup and close the file
    fclose(traceFile);

    // Cleanup resources
    // printf("interconnect in main %p\n", (void*)interconnect);
    // printf("incomingQueue in main %p\n", (void*)interconnect->incomingQueue);
    freeInterconnect();

    return 0;
}