
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "volumeCB.h"
#include "bitmap.h"
#include "fsLow.h"
#include "rootDir.h"
#include "mfs.h"

#define BLOCK_SIZE 512



int initDir(int minEntries, DE* parent) {
    int minBytesNeeded = minEntries * sizeof(DE); // Calculate the minimum number of bytes needed to accommodate the specified number of directory entries
    int blocksNeeded = (minBytesNeeded + BLOCK_SIZE - 1) / BLOCK_SIZE; // Calculate the number of blocks needed to store the directory entries, considering the block size
    int bytesToAlloc = blocksNeeded * BLOCK_SIZE; // Calculate the total number of bytes to allocate for the directory
    int actualEntries = bytesToAlloc / sizeof(DE); // Calculate the actual number of directory entries that can fit in the allocated space
    DE* newDir = (DE*)malloc(bytesToAlloc); // Dynamically allocate memory for the directory entries

    int i;
    for (i = 2; i < actualEntries; i++) {
        // Initialize each entry
        strcpy(newDir[i].name, "File");  // Set a generic name for each file
        newDir[i].size = 0;
        newDir[i].loc = 0;
    }

    // Set "." entry
    strcpy(newDir[0].name, ".");
    newDir[0].size = actualEntries * sizeof(DE);
    blockAllocation ba;  // Placeholder for allocateBlocks function
    newDir[0].loc = ba.start;

    
    strcpy(newDir[1].name, "..");
    if (parent == NULL) { // since "." and ".." are same except for one
        parent = newDir;  // also reduces code redundancy like we discussed in class
    }
    newDir[1].size = parent[0].size;
    newDir[1].loc = parent[0].loc;

    // Write the new directory to disk
    LBAwrite(newDir, blocksNeeded, newDir[0].loc);

    return newDir[0].loc;
}
