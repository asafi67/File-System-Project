/**************************************************************
* Class:  CSC-415-01 Summer 2023
* Names:Anish Khadka, Joe Sand
* Student IDs:921952002,920525382
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

void releaseBlocks(int start, int count, int bytesPerBlock){
		
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
    int sizeOfBitmap = (count + 7)/8;

    // LBAwrite writes the i-th block of the bitmap to the i-th block on the disk
    // the memory address is calculated by adding i * bytesPerBlock to the bitmap pointer
    // 1 block of data is written starting from the block position 1 + i on the disk
    // if the function returns something other than 1, it indicates an error
    int blocksNeeded = (sizeOfBitmap + bytesPerBlock - 1)/bytesPerBlock;
    for(int i = 0; i < blocksNeeded; i++){
        if(LBAwrite(bitmap + i * bytesPerBlock, 1, 1 + i) != 1){
            printf("bitmap failed to write to disk");
        }
    }
}

//when there already is initialized VCB.
int loadFreeSpace(int blockCount, int bytesPerBlock){

	//determine the size of the bitmap
	int bitmapSize = ((blockCount+7)/8);

	//allocate enough memory to hold the bitmap
	bitmap = (char*) malloc(bitmapSize);
	
	//check if bitmap failed to malloc
	if(bitmap == NULL){
		printf("bitmap failed");
		return -1;
		}
	
	//calculate how many blocks the bitmap spans
	uint64_t blocks = (bitmapSize + bytesPerBlock -1)/bytesPerBlock;
	
	//loop through the bits
	for(uint64_t i = 0; i < blocks; i++){
		
		//attempt to read the block
		if (LBAread(bitmap + i * bytesPerBlock, 1, 1 +i) != 1){
			//if it fails free the allocated memory
            printf("failed to load freespace");
			free(bitmap);
			return -1;
		}
		//else re-enter loop and continue loading
	}

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
    
    //calculate the size of the bitmap in bytes
    int bitmapSize = (blockCount + 7) / 8; 
    
    //calculate how many blocks are needed to store the bitmap, rounding up
	int blocksNeeded = (bitmapSize + bytesPerBlock - 1) / bytesPerBlock; 
	
    for (int i = 0; i < blocksNeeded; i++) {
		

        //attempt to write the bitmap block to the disk using LBAwrite
        if (LBAwrite(bitmap + i * bytesPerBlock, 1, 1 + i) != 1) {
			
            //if it fails free the allocated bitmap memory
            printf("Failed to write bitmap to disk");
            free(bitmap);
            bitmap = NULL; 
			return -1;
		}
	}
	//exit
	return 0;
}

blockAllocation allocateBlocks (int required, int minPerExtent){
	blockAllocation ba;
   
    ba.start = -1; //setting the start block of the allocation to -1
                   //if still -1 after the function, no sequence was found
    ba.count = 0;  //setting the count of allocated block to 0
                   //if still 0 after the function, no blocks were allocated
    
    int bytesPerBlock = 8;
    int bitmapSize = sizeof(bitmap);
    int foundSpace = 0;
   
    //search bitmap for a sequence of of continuous free space blocks here
    for(int i = 0; i < bitmapSize; i++){
        for(int j = 0; j < bytesPerBlock; j++){
            
            //check if block free
            if(!(bitmap[i] & (1 << j))) {
                if(ba.start == -1){ // check if first free block
                ba.start = i * bytesPerBlock + j;
            }

            foundSpace++; //increment count of found free blocks
                
            // check if we hit the required ammount of free blocks
            if(foundSpace == required){

                ba.count = foundSpace; //set count to the count found

                //loop to mark these blocks as allocated in bitmap
                for(int k = 0; k < foundSpace; k++){
                    //bitwise OR assignment 
                    bitmap[ba.start / bytesPerBlock + k/bytesPerBlock] |=
                    (1<<(ba.start % bytesPerBlock + k % bytesPerBlock));
                }

            // Calculate the number of blocks required to store the bitmap.
            // rounding up, dividing the size of the bitmap by the number of bytes per block, 
            int blocksNeeded = (bitmapSize + bytesPerBlock - 1) / bytesPerBlock;
                    
            // loop iterates over each of the blocks needed to store the bitmap
             for(int y = 0; y < blocksNeeded; y++){
                        
                //  an attempt to write each block of the bitmap to the disk using the LBAwrite
                // 'bitmap + y * bytesPerBlock' calculates the memory address of the block 
                //  y is the number of blocks to write.
                //  y + 1 is the starting LBA (Logical Block Address) to write to.
                if(LBAwrite(bitmap + y * bytesPerBlock, 1, 1 + y) != 1){
                            
                    // if the write to the disk failed than we're inside the if
                    printf("bitmap failed to write to disk");
                    // set start and count to indicate failure and return the structure
                    ba.start = -1;
                    ba.count = 0;
                    return ba;
                }
            }
            //return the start block and count
            return ba;

            }

        } else {
            //block requested is not free 
            ba.start = -1; //reset the sequence
            foundSpace = 0;
            }
        }
    }

    // if here, we couldnt find enough free space
	printf("Could not allocatre enough free blocks\n");
    ba.start = -1;
	ba.count = 0;
	return ba;
}

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

    // We dynamically allocate memory for the volume control block structure
    VolumeControlBlock* vcb = (VolumeControlBlock*)malloc(sizeof(VolumeControlBlock));

    // We read the content of block 0 from the disk using logical block addressing
    LBAread(0, vcb, sizeof(VolumeControlBlock));

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
