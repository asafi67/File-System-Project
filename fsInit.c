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
		printf("malloc for VCB failed");
		return -1;
	}
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
		
		//write the volume control block to disc
		if(LBAwrite(vcb, 1, 0) != 1){
			printf("error occurred writing vcb to disk in fs_init.\n");
			return -1;
		}
		
	
	
	
	} else {
		
		printf("File system is already initialized\n");
	}
		//allocate memory for the extern cwdPointer
	cwdPointer = malloc(sizeof(DirectoryEntry) * BUFFER_SIZE + vcb->blockSize -1);
	if(cwdPointer==NULL){
		printf("malloc failed in fsInit!\n");
		return -1;
	}
	//define the extern cwdPointer to start pointing at the root directory array
	LBAread(cwdPointer, (sizeof(DirectoryEntry) * BUFFER_SIZE + vcb->blockSize -1) / vcb->blockSize, vcb->rootIndex );

	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}	