/**************************************************************
* Class:  CSC-415-0# Fall 2021
* Names: 
* Student IDs:
* GitHub Name:
* Group Name:
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

typedef struct {
	unsigned int numOfBlocks;
	unsigned blockSize;
	unsigned int freeBlocks;
	unsigned int allocatedBlocks;
} VolumeControlBlock;

//global bitmap
char* bitmap;

//structure for free space block allocation 
typedef struct{
	//start of the block
	int start;
	//count or number of continuous blocks
	int count;
}blockAllocation;

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	
	//Mallock a block of memory as VCB pointer
	VolumeControlBlock* vcb = (VolumeControlBlock*)malloc(sizeof(VolumeControlBlock));

	//LBA read block 0
	LBAread(0, vcb, sizeof(VolumeControlBlock));

	
	if(vcb->numOfBlocks != 0 || vcb->blockSize != 0){
	//If the volume control block has already been initialized
	//We can put the code for that case in this section

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



	}else{
		//Following code is what we do if VCB has not been initialized already

		//Initialize values in volume control block
		vcb->numOfBlocks = numberOfBlocks;
		vcb->blockSize = blockSize;
		vcb->freeBlocks = numberOfBlocks;
		vcb->allocatedBlocks = 0;

		//Initialize free space
		int initFreeSpace(int blockCount, int bytesPerBlock){
		
		//allocate enough space in bitmap, rounding up to nearest byte
		bitmap = malloc((blockCount + 7)/8)
		
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
			ba.start = ;
			ba.count = ;
			return ba;
		}

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
		//Initialize the root directory

		//Set the values returned from free space and root directory in the VCB
		//LBAwrite the VCB to block 0



	}


	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}