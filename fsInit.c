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


	}else{
		//Following code is what we do if VCB has not been initialized already

		//Initialize values in volume control block
		vcb->numOfBlocks = numberOfBlocks;
		vcb->blockSize = blockSize;
		vcb->freeBlocks = numberOfBlocks;
		vcb->allocatedBlocks = 0;

		//Initialize free space
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