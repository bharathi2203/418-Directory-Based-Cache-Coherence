/**
 * @file csim.c
 * @brief Cache Simulator
 * 
 * @author BHARATHI SRIDHAR <bsridha2@andrew.cmu.edu>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include "single_cache.h"

/**
 * @brief Struct representing each line/block in a cache
 * 
 * Lines are implemented as linked lists.
*/
typedef struct line {
    int lineNum;                            // represents line number
    bool valid;                             // represents valid bit
    unsigned long int tag;                  // represents tag bits
    struct line *nextLine;                  // ptr to next line in same set
    bool isDirty;                           // represents dirty bit for each cache line
} line_t;

/**
 * @brief Struct representing each set in a cache
 * 
*/
typedef struct set {
    struct line *topLine;                     // head of linked list of lines in set
    unsigned long linesFilled;                // tracks occupancy of set
} set_t;

/**
 * @brief Struct representing the cache
 * 
*/
typedef struct {
    unsigned long S;                          // Number of set bits
    unsigned long E;                          // Associativity: number of lines per set
    unsigned long B;                          // Number of block bits
    unsigned long hitCount;                   // number of hits
    unsigned long missCount;                  // number of misses
    unsigned long evictionCount;              // number of evictions
    unsigned long dirtyEvictionCount;         // number of evictions of dirty lines
    struct set **setList;                     // Array of Sets
} cache; 

/**
 * @brief Adds a new line to the linked lists of lines
 *        from a particular set.
 * 
*/
cache *addLine(cache *C, long setNo, int lineNo) {
    struct set *currSet = C->setList[setNo];
    struct line *new = malloc(sizeof(struct line));
    new->isDirty = false;
    new->tag = 0;
    new->valid = false; 
    new->nextLine = NULL;
    new->lineNum = lineNo;
    struct line *curr = currSet->topLine;
    if (curr == NULL) {
        currSet->topLine = new;
    }
    else {
        while (curr->nextLine != NULL) {
            curr = curr->nextLine;
        }
        curr->nextLine = new; 
    }
    currSet->linesFilled += 1;
    return C;
}

/**
 * @brief Removes line and adds it to end of linked list
 *        given the set number and line number.
 * 
*/
cache *removeLine(cache *C, long setNo, int lineNo) {
    struct set *currSet = C->setList[setNo];
    if (currSet->linesFilled == 1) {
        return C;
    }
    struct line *remLine; 
    // Eviction case: remove topline, add it to end
    if (currSet->topLine->lineNum == lineNo) {
        remLine = currSet->topLine;
        currSet->topLine = currSet->topLine->nextLine;
        struct line *currLine = currSet->topLine;
        while(currLine->nextLine != NULL) {
            currLine = currLine->nextLine;
        }
        currLine->nextLine = remLine;
        remLine->nextLine = NULL;
        return C;
    }
    // hit/miss case: requires lines to be reordered
    struct line *currLine = currSet->topLine;
    while (currLine->nextLine != NULL) {
        if (currLine->nextLine->lineNum == lineNo) {
            remLine = currLine->nextLine;
            currLine->nextLine = currLine->nextLine->nextLine;
        }
        else {
            currLine = currLine->nextLine;
        }
    }
    remLine->nextLine = NULL;
    currLine->nextLine = remLine;
    return C;
}

/**
 * @brief Returns first available line number for which there isn't 
 *        an allocated line in the set. 
*/
int getLineNum(cache *C, long setNo) {
    struct set *currSet = C->setList[setNo];
    for (int i = 0; i < (int)(C->E); i++) {
        struct line *currLine = currSet->topLine;
        if (currLine == NULL) {
            return 0;
        }
        while (currLine->nextLine != NULL) {
            if (currLine->lineNum == i) break;
            currLine = currLine->nextLine;
        }
        if (currLine->lineNum != i) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Create the cache with the parameters parsed. 
 * @param[in]       s       number of set bits in the address
 * @param[in]       e       number of lines in each set
 * @param[in]       b       number of block bits in the address
 * @param[out]      new     newly allocated Cache.
 * 
*/
cache *newCache(unsigned int s, unsigned int e, unsigned int b) {
    unsigned int S = 1U << s;
    if (e == 0) {
        //fprintf(stderr, "Invalid arguements.");
        return NULL;
    }
    cache *new = malloc(sizeof(cache));
    if (new == NULL){
        return NULL;
    }
    new->S = s;
    new->E = e;
    new->B = b;
    new->hitCount = 0;
    new->missCount = 0;
    new->evictionCount = 0;
    new->dirtyEvictionCount = 0;
    //Initialize sets
    struct set **sets  = malloc(S * sizeof(struct set *));
    for(unsigned int i = 0; i < S; i++) {
        sets[i] = malloc(sizeof(struct set));
    }
    for(unsigned int i = 0; i < S; i++) {
        struct set *currSet = sets[i];
        currSet->linesFilled = 0;
        currSet->topLine = NULL;
    }
    new->setList = sets;
    return new;     
}

/**
 * @brief Function prints every set, every line in the Cache.
 *        Useful for debugging!
 * 
*/
void printCache(cache *C) {
    fprintf(stderr, "\n\t\t\t\tPRINTING CACHE:");
    struct set **sets = C->setList;
    unsigned int S = 1U << (C->S);
    for(unsigned int i = 0; i < S; i++) {
        struct set *currSet = sets[i];
        fprintf(stderr, "\nSET: %d \t Lines Filled: %ld", i, currSet->linesFilled);
        struct line *currLine = currSet->topLine;
        if (currLine == NULL) {
            fprintf(stderr, "\tSET EMPTY! ");
        }
        else {
            while (currLine != NULL) {
                fprintf(stderr, "\nLineNum: %d \t V: %d \t Tag: %lu \t DirtyBit: %d", currLine->lineNum, currLine->valid, currLine->tag, currLine->isDirty);
                currLine = currLine->nextLine;
            }
        }
    }
    fprintf(stderr, "\n\n\n\n");
}

/**
 * @brief Performs the L or Load operation.
 * 
*/
cache *loadUpdate(cache* C, unsigned long int addr, int size) {
    //set selection
    long address = (long int)addr;
    long mask = (((~0L << (C->S + C->B))) & (~0L << (C->B))) ^ (~0L << (C->B));
    long setNumber = ((address & mask) >> (C->B));
    //line matching - is the word already in this set
    bool isCacheHit = false;
    struct line *currLine = (C->setList[setNumber])->topLine;
    while (currLine != NULL) {
        if (currLine->valid && currLine->tag == (long unsigned)(address >> (C->S + C->B))) { 
            isCacheHit = true; 
            C = removeLine(C, setNumber, currLine->lineNum);
            break;
        }
        currLine = currLine->nextLine;
    }
    if (isCacheHit) {
        C->hitCount += 1;
    }
    else {
        //Find available line in set
        struct line *freeLine;
        C->missCount += 1;
        if (C->setList[setNumber]->linesFilled < C->E) {
            int newLineNum = getLineNum(C, setNumber); //get earliest available line number
            assert(newLineNum != -1);
            C = addLine(C, setNumber, newLineNum);
            freeLine = (C->setList[setNumber])->topLine;
            while (freeLine->nextLine != NULL) {
                freeLine = freeLine->nextLine;
            }
            assert(freeLine->lineNum == newLineNum);
        }
        else {
            C = removeLine(C, setNumber, ((C->setList[setNumber])->topLine)->lineNum);
            freeLine = (C->setList[setNumber])->topLine;
            while (freeLine->nextLine != NULL) {
                freeLine = freeLine->nextLine;
            }
            C->evictionCount += 1;
            if (freeLine->isDirty) {
                C->dirtyEvictionCount += 1;
            }
        }
        freeLine->valid = true;
        freeLine->tag = (long unsigned)(address >> (C->S + C->B));
        freeLine->isDirty = false;
    }
    return C;
}

/**
 * @brief Performs the S or Store operation.
 * 
*/
cache *storeUpdate(cache* C, unsigned long int addr, int size) {
    //set selection
    long address = (long int)addr;
    long mask = (((~0L << (C->S + C->B))) & (~0L << (C->B))) ^ (~0L << (C->B));
    long setNumber = ((address & mask) >> (C->B));
    //line matching - is the word already in this set
    bool isCacheHit = false;
    struct line *currLine = (C->setList[setNumber])->topLine;
    while (currLine != NULL) {
        if (currLine->valid && currLine->tag == (long unsigned)(address >> (C->S + C->B))) { 
            isCacheHit = true; 
            C = removeLine(C, setNumber, currLine->lineNum);
            currLine->isDirty = true;
            break;
        }
        currLine = currLine->nextLine;
    }
    if (isCacheHit) {
        C->hitCount += 1;
    }
    else {
        //Find available line in set
        struct line *freeLine;
        C->missCount += 1;
        if (C->setList[setNumber]->linesFilled < C->E) {
            int newLineNum = getLineNum(C, setNumber); //get earliest available line number
            assert(newLineNum != -1);
            C = addLine(C, setNumber, newLineNum);
            freeLine = (C->setList[setNumber])->topLine;
            while (freeLine->nextLine != NULL) {
                freeLine = freeLine->nextLine;
            }
            assert(freeLine->lineNum == newLineNum);
        }
        else {
            C = removeLine(C, setNumber, ((C->setList[setNumber])->topLine)->lineNum);
            freeLine = (C->setList[setNumber])->topLine;
            while (freeLine->nextLine != NULL) {
                freeLine = freeLine->nextLine;
            }
            C->evictionCount += 1;
            if (freeLine->isDirty) {
                C->dirtyEvictionCount += 1;
            }
        }
        freeLine->valid = true;
        freeLine->tag = (long unsigned)(address >> (C->S + C->B));
        freeLine->isDirty = true;
    }
    return C;
}

/**
 * @brief Parses tracefile line by line and update cache fields accordingly.
 * 
 * General idea:
 *      1. read tracefile line by line
 *      2. implement instruction from each line in cache
 *      3. update cache parameters after inplementing each line
 * 
*/
bool updateCache(cache *C, char* tracefilePath) { 
    FILE* tracefile = fopen(tracefilePath, "r"); 
    if (tracefile == NULL) {
        fprintf(stderr,"COULD NOT OPEN TRACEFILE! <updateCache fn>"); 
        fclose(tracefile);
        return false;
    }
    //From 18-213 Recitation 5 slides.
    char access_type;
    unsigned long address;
    int size;
    while (fscanf(tracefile, " %c %lx, %d", &access_type, &address, &size) > 0) {
        if (access_type == 'L') { //L =>READ
            C = loadUpdate(C, address, size);
        }
        else if (access_type == 'S') { //S => WRITE
            C =  storeUpdate(C, address, size);
        }
        else {
            fprintf(stderr,"INVALID ACCESS TYPE IN TRACE FILE.");
            break;
        }
    } 
    fclose(tracefile);
    return true; 
}

/**
 * @brief Frees all memory allocated to the cache,
 *        including memory allocated after initialization
 *        such as the lines added.
 * 
*/
void freeCache(cache *C) {
    struct set **sets = C->setList;
    int S = (1 << C->S);
    for (int i = 0; i < S; i++) {
        struct set *currSet = sets[i];
        struct line *prev = currSet->topLine;
        struct line *curr = currSet->topLine;
        while(prev != NULL) {
            curr = prev->nextLine;
            free(prev);
            prev = curr;
        }
        free(currSet);
    }
    free(sets);
    free(C);
}

/**
 * @brief Prints information about what parameters the program requires and it's format.
 * 
*/
void displayUsage() {
    fprintf(stderr,"\nUSAGE GUIDE"); 
    fprintf(stderr, "\n");
    fprintf(stderr, "\nUsage:\t\t ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>");
    fprintf(stderr, "\n-h:\t Optional help flag that prints usage info");
    fprintf(stderr, "\n-v:\t Optional verbose flag that displays trace info");
    fprintf(stderr, "\n-s <s>:\t Number of set index bits (S = 2^s is the number of sets)");
    fprintf(stderr, "\n-E <E>:\t Associativity (number of lines per set)");
    fprintf(stderr, "\n-b <b>:\t Number of block bits (B = 2^b is the block size)");
    fprintf(stderr, "\n-t <tracefile>:\t Name of the memory trace to replay");
}

/**
 * @brief Makes a "summary" of stats based on the fields of the cache. 
 *        This function is essentially a helper function to create the 
 *        csim_stats_t struct that the printSummary fn requires.
 * 
*/
const csim_stats_t *makeSummary(cache* C) {
    csim_stats_t *stats = malloc(sizeof(csim_stats_t));
    stats->hits = C->hitCount;
    stats->misses = C->missCount;
    stats->evictions = C->evictionCount;

    unsigned long dirtyByteCount = 0;
    //Loop through all lines, count all dirty lines
    struct set **sets = C->setList;
    unsigned int S = 1U << C->S;
    for(unsigned int i = 0; i < S; i++) {
        struct set *currSet = sets[i];
        struct line *currLine = currSet->topLine;
        while (currLine != NULL) {
            if (currLine->isDirty) {
                dirtyByteCount += 1;
            }
            currLine = currLine->nextLine;
        }
    }
    stats->dirty_bytes = dirtyByteCount * (1 << C->B);
    stats->dirty_evictions = C->dirtyEvictionCount * (1 << C->B);
    const csim_stats_t *result = stats;
    return result;
}


int main(int argn, char *args[]) {
    int opt;
    unsigned int s;
    unsigned int e;
    unsigned int b; 
    char* tracefilePath;
    if (argn < 2) {
        fprintf(stderr, "Insufficient args.");
        return 1; 
    }
    while ((opt = getopt(argn, args, "hvs:E:b:t:")) != -1) { 
        switch (opt) {
        case 's':
            s = (unsigned int)atoi(optarg); 
            break; 
        case 'E':
            e = (unsigned int)atoi(optarg); 
            break;
        case 'b':
            b = (unsigned int)atoi(optarg); 
            break;
        case 't':
            tracefilePath = malloc(200);
            tracefilePath = strcpy(tracefilePath, optarg); 
            break;
        case 'v': 
            break;
        case 'h': 
            displayUsage(); 
            break;
        default:
            fprintf(stderr, "Invalid inputs.");
            displayUsage();
            break;
        }
    }
    cache *myCache = newCache(s, e, b); 
    if (myCache == NULL || tracefilePath == NULL) {
        return 1;
    }
    bool didItUpdate = updateCache(myCache, tracefilePath);
    if (!didItUpdate) return 1;
    const csim_stats_t *stats = makeSummary(myCache);
    printSummary(stats);
    free((void*)stats);
    free(tracefilePath);
    freeCache(myCache); 
    return 0; 
}
