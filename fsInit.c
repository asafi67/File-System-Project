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



//signature of file system checked to see if fs is mounted
#define MAGIC_NUMBER 415001    //ameen set this
//maximum number of chars for a file path in our fs
#define MAX_PATH_LEN 500
//declaring vcb
VolumeControlBlock* vcb;
//declare a bitmap blocksize
int bitmapBlockSize;
//declare bitmap
unsigned char* bitmap;
//returns -1 on failure 0 on success
DE* currentDirectoryPointer;
int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	
	//pull in the VCB from disc 
	vcb = malloc( blockSize );
	if(vcb == NULL){
		printf("Memory allocation failed for VCB");
		return -1;
	}
	//error check 
	if(LBAread(vcb, 1, 0) != 1){
		printf("LBAread did not read in vcb in initFileSystem function\n");
		return -1;
	}
	
	//check that the magicNumber is a match
	if(vcb->magic_number != MAGIC_NUMBER){ //the fs has not been initialized yet 
		//so initialize it
		printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
		
		//initialize the vcb
		vcb->magic_number = MAGIC_NUMBER;
		vcb->block_size = blockSize;
		vcb->freeBlockCount = numberOfBlocks;
		//initialize the freespace management
		int indexOfBitmap = bitmapInit(numberOfBlocks, blockSize);
		//initialize the rootDirectory
		int indexOfRoot = initDir(NULL, blockSize);
		//initalize the rest of the vcb
		vcb->bitmap_index = indexOfBitmap;
		vcb->root_index = indexOfRoot;
		vcb->num_blocks = numberOfBlocks;
		
		//write VCB to disc
		if(LBAwrite(vcb, 1, 0) != 1){
			//if LBAwrite doesn't return 1 than print error
			printf("error occurred writing vcb to disk in fs_init.\n");
			return -1;
		}
	} else {
		//if else than fs is initialized
		printf("File system is already initialized\n");
	}
	
	// Allocate memory for the external current directory pointer
    currentDirectoryPointer = malloc(sizeof(DE) * BUFFER_SIZE +
	 vcb->block_size - 1);
    if (currentDirectoryPointer == NULL) {
        printf("Memory allocation failed in fsInit!\n");
        return -1;
    }

	// Define the external current directory pointer to start at the root directory array
    LBAread(currentDirectoryPointer, (sizeof(DE) * BUFFER_SIZE +
	 vcb->block_size - 1) 
	/ vcb->block_size, vcb->root_index);

	return 0;
}
	
// Function to exit the file system
void exitFileSystem (){
	printf ("System exiting\n");
}	
