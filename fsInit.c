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

//this is a signature for the file system in order to check whether fs is mounted
#define MAGIC_NUMBER 415001

 // We dynamically allocate memory for the volume control block structure
    VolumeControlBlock* vcb;

  //Declare the block size for the bitmap
    int bitmapBS;

//Declare the bitmap itself
  extern char* bitmap;

//create current working directory pointer
DE* cwdPointer;

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    vcb = malloc(blockSize);
    if(vcb == NULL){
        printf("malloc for VCB failed");
        return -1;
    }
    if(LBAread(vcb, 1, 0) != 1){
        printf("LBAread did not read in vcb in initFileSystem function\n");
        return -1;
    }
    
    
    
    
    printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

    // We read the content of block 0 from the disk using logical block addressing
    //LBAread(vcb, 0, sizeof(VolumeControlBlock));

	//First condition is in place to check if the VCB has already been initialized
    if (vcb->magicNumber == MAGIC_NUMBER) {
    

        // // TODO: Load free space
        loadFreeSpace(numberOfBlocks,blockSize);
        printf("File system is already initialized\n");

    } else {
        // Following code is what we do if VCB has not been initialized already
        printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
        // Initialize values in volume control block
        vcb->magicNumber = MAGIC_NUMBER;
        vcb->numOfBlocks = numberOfBlocks;
        vcb->blockSize = blockSize;
        vcb->freeBlocks = numberOfBlocks;
        vcb->allocatedBlocks = 0;

        // Initialize free space
        int index = initFreeSpace(numberOfBlocks,blockSize);

        // Function declaration for initDir
        int initDir(int minEntries, DE* parent);

        // Initialize the root directory
        int rootStartBlock = initDir(50, NULL);

        // Set the values returned from free space and root directory in the VCB
        vcb->freeBlocks -= 6;  // Subtract 6 blocks used for VCB, bitmap, and root directory
        vcb->allocatedBlocks += 6;
        vcb->allocatedBlocks += initFreeSpace(numberOfBlocks, blockSize);
        vcb->allocatedBlocks += rootStartBlock;
        vcb->bitmap_index = index;
        vcb -> root_index = rootStartBlock;

        // LBA write the VCB to block 0
       (LBAwrite(vcb, sizeof(VolumeControlBlock), 0));

       printf("allocated blocks: %d\n", rootStartBlock);

       cwdPointer = malloc(sizeof(DE)*BUFFER_SIZE + vcb->blockSize -1);
       if(cwdPointer == NULL){
        printf("malloc failed in fsInit!\n");
        return -1;
       }
       LBAread(cwdPointer, (sizeof(DE)*BUFFER_SIZE + vcb->blockSize-1)/vcb->blockSize, vcb->root_index);
    }

    


    return 0;
}

void exitFileSystem() {
    printf("System exiting\n");
}

	