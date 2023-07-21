/**************************************************************
* Class:  CSC-415-01 Summer 2023
* Names:Anish Khadka, Joe Sand, Ameen Safi
* Student IDs:921952002,920525382, 920689065
* GitHub Name:
* Group Name: File System Soldiers
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "rootDir.h"
#include "bitmap.h"
#include "volumeCB.h"


int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

    // We dynamically allocate memory for the volume control block structure
    VolumeControlBlock* vcb = (VolumeControlBlock*)malloc(sizeof(VolumeControlBlock));

    // We read the content of block 0 from the disk using logical block addressing
    LBAread(vcb, 0, sizeof(VolumeControlBlock));

	//First condition is in place to check if the VCB has already been initialized
    if (vcb->numOfBlocks != 0 || vcb->blockSize != 0) {
    

        // TODO: Load free space
        loadFreeSpace(numberOfBlocks,blockSize);

    } else {
        // Following code is what we do if VCB has not been initialized already

        // Initialize values in volume control block
        vcb->numOfBlocks = numberOfBlocks;
        vcb->blockSize = blockSize;
        vcb->freeBlocks = numberOfBlocks;
        vcb->allocatedBlocks = 0;

        // Initialize free space
        initFreeSpace(numberOfBlocks,blockSize);

        // Function declaration for initDir
        int initDir(int minEntries, DE* parent);

        // Initialize the root directory
        int rootStartBlock = initDir(50, NULL);

        // Set the values returned from free space and root directory in the VCB
        vcb->freeBlocks -= 6;  // Subtract 6 blocks used for VCB, bitmap, and root directory
        vcb->allocatedBlocks += 6;
        vcb->allocatedBlocks += initFreeSpace(numberOfBlocks, blockSize);
        vcb->allocatedBlocks += rootStartBlock;

        // LBA write the VCB to block 0
        LBAwrite(vcb, sizeof(VolumeControlBlock), 0);
    }

    return 0;
}

void exitFileSystem() {
    printf("System exiting\n");
}
