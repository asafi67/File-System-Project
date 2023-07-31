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
#define MAX_PATH_LEN 400
//declaring vcb
VCB* vcb;
//declare a bitmap blocksize
int bitmapBlockSize;
//declare bitmap
unsigned char* bitmap;
//returns -1 on failure 0 on success
DirectoryEntry* cwdPointer;
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
	if(vcb->magicNumber != MAGIC_NUMBER){ //the fs has not been initialized yet 
		//so initialize it
		printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
		
		//initialize the vcb
		vcb->magicNumber = MAGIC_NUMBER;
		vcb->blockSize = blockSize;
		vcb->freeBlockCount = numberOfBlocks;
		//initialize the freespace management
		int indexOfBitmap = bitmapInit(numberOfBlocks, blockSize);
		//initialize the rootDirectory
		int indexOfRoot = initDirectory(NULL, blockSize);
		//initalize the rest of the vcb
		vcb->bitmapIndex = indexOfBitmap;
		vcb->rootIndex = indexOfRoot;
		vcb->numBlocks = numberOfBlocks;
		
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
    currentDirectoryPointer = malloc(sizeof(DirectoryEntry) * BUFFER_SIZE +
	 vcb->blockSize - 1);
    if (currentDirectoryPointer == NULL) {
        printf("Memory allocation failed in fsInit!\n");
        return -1;
    }

	// Define the external current directory pointer to start at the root directory array
    LBAread(currentDirectoryPointer, (sizeof(DirectoryEntry) * BUFFER_SIZE +
	 vcb->blockSize - 1) 
	/ vcb->blockSize, vcb->rootIndex);

	return 0;
}
	
// Function to exit the file system
void exitFileSystem (){
	printf ("System exiting\n");
}	
