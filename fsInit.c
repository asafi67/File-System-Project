/**************************************************************
* Class:  CSC-415-01 Summer 2023
* Names:Anish Khadka, 
* Student IDs:921952002, 
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

#define BLOCK_SIZE 512

typedef struct {
    unsigned int numOfBlocks;
    unsigned int blockSize;
    unsigned int freeBlocks;
    unsigned int allocatedBlocks;
} VolumeControlBlock;

// Structure for directory entry
typedef struct {
    char name[256];
    unsigned int size;
    unsigned int loc;
} DE;

// Global bitmap
char* bitmap;

// Structure for free space block allocation
typedef struct {
    int start;
    int count;
} blockAllocation;

void releaseBlocks(int start, int count){
		
	//update the bitmap to mark the blocks as free 0x00
	for (int i = 0; i < count; i++){
			
	//where the bit for this block is located in the bitmap
	int index = (start + i)/8;
			
	//acquire bit position between 0-7
	int bit = (start + i) % 8;

	//set bit to zero
	bitmap[index] &= ~(1 << bit);
	}
		
	//write the updated bitrmap back to the disk

}

//when there already is initialized VCB.
int loadFreeSpace(int blockCount, int bytesPerBlock){
	
	//load the bitmap from the volume here

	//determine the size of the bitmap
	int bitmapSize = ((blockCount+7)/8);

	//allocate enough memory to hold the bitmap
	char* bitmap = malloc(bitmapSize);
	
	//check if bitmap failed to malloc
	if(bitmap == NULL){
		printf("bitmap failed");
		return -1;
		}
	
	//load the freespace
	uint64_t blocks = (bitmapSize + bytesPerBlock -1)/bytesPerBlock;
	
	//loop through the bits
	for(uint64_t i = 0; i < blocks; i++){
		
		//error check
		if (LBAread(bitmap + i * bytesPerBlock, 1, 1 +i) != 1){
			printf("failed to load freespace");
			free(bitmap);
			return -1;
		}
		//else re-enter loop and continue loading
	}
	
	//write the bitmap to disk here
	
	//exit
	return 0;
}

//Initialize free space
int initFreeSpace(int blockCount, int bytesPerBlock){
		
	//allocate enough space in bitmap, rounding up to nearest byte
	bitmap = malloc((blockCount + 7)/8);
		
	//error check
	if(!bitmap){
		printf("bitmap failed to allocate");
		return -1;
	}

	//setting all bits to 0 meaning free
	memset(bitmap, 0x00, (blockCount +7)/8);

	//write the bitamp to the disk here

	//exit
	return 0;
}

blockAllocation allocateBlocks (int required, int minPerExtent){
	blockAllocation ba;
	//search bitmap for a sequence of of continuous free space blocks here

	//once a sequence of continuous free space is found, update the bitmap

	//write updated bitmap back to the disk


	//return the start block and count
	ba.start = 0;
	ba.count = 0;
	return ba;
}

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

    // Malloc a block of memory as VCB pointer
    VolumeControlBlock* vcb = (VolumeControlBlock*)malloc(sizeof(VolumeControlBlock));

    // LBA read block 0
    LBAread(0, vcb, sizeof(VolumeControlBlock));

    if (vcb->numOfBlocks != 0 || vcb->blockSize != 0) {
        // If the volume control block has already been initialized
        // We can put the code for that case in this section

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

void exitFileSystem() {
    printf("System exiting\n");
}
