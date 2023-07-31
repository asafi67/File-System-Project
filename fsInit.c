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



//we define a signature for the fs to be used for checking if it is mounted
#define MAGIC_NUMBER 415001    //ameen set this
//max character count for any path in our fs
#define MAX_PATH_LEN 400
//we declare a volume control block
VolumeControlBlock* vcb;
//declare a block size for bitmap
int bitmap_block_size;
//declare bitmap
unsigned char* bitmap;
//returns -1 on failure 0 on success
//This pointer denotes a current working directory
DE* cwdPointer;
int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	
	//extract the vcb from disk
	vcb = malloc( blockSize );
	if(vcb == NULL){
		printf("Memory allocation failed for VCB");
		return -1;
	}
	if(LBAread(vcb, 1, 0) != 1){
		printf("LBAread did not read in vcb in initFileSystem function\n");
		return -1;
	}
	
	//check that the vcb's magic# field matches the #define magic#
    //to determine if the vcb has been initialized or not
	//this 1st condition would indicate fs has not been initialized yet 
    if(vcb->magic_number != MAGIC_NUMBER){ 
		//We then initialize it in this case
		printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
		
		//Initialize the vcb
		vcb->magic_number = MAGIC_NUMBER;
		vcb->block_size = blockSize;
		vcb->freeBlockCount = numberOfBlocks;
		
        //Initialize the freespace management using bitmap function
		int indexOfBitmap = bitmapInit(numberOfBlocks, blockSize);
		
        //Initialize the root directory 
		int indexOfRoot = initDir(NULL, blockSize);
		
        //Tnitalize the remaining parts of the vcb
		vcb->bitmap_index = indexOfBitmap;
		vcb->root_index = indexOfRoot;
		vcb->num_blocks = numberOfBlocks;
		
		//write the volume control block to disc
		if(LBAwrite(vcb, 1, 0) != 1){
			printf("Unable to write the vcb onto disc\n");
			return -1;
		}
		
	
	
	
	} else {
		
		printf("File system is already initialized\n");
	}
		//We have to allocate memory for the cwdPointer extern
	cwdPointer = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size -1);
	if(cwdPointer==NULL){
		printf("Unable to allocate memory in fsInit!\n");
		return -1;
	}
	//Assign the root directory array to the extern pointer as the thing it points to
	LBAread(cwdPointer, (sizeof(DE) * BUFFER_SIZE + vcb->block_size -1) / vcb->block_size, vcb->root_index );

	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}	