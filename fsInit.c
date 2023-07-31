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



#define MAGIC_NUMBER 415001    //ameen set this
#define MAX_PATH_LEN 400 //maximum characters in a file path in our fs


VCB* vcb; //declaring vcb
int sizeOfBlock; //declare a block size for the bitmap
unsigned char* bitmap; //declare bitmap
DirectoryEntry* currentDirectoryPointer; // declare current directory pointer

int initFileSystem (uint64_t totalBlocks, uint64_t sizeOfBlock){
	
	//Retrieve the VCB from the disk
	vcb = malloc( sizeOfBlock );
	
	//error check
	if(vcb == NULL){
		printf("malloc for VCB failed");
		return -1;
	}
	//error check 
	if(LBAread(vcb, 1, 0) != 1){
		printf("LBAread did not read in vcb in initFileSystem function\n");
		return -1;
	}
	
	//verify the magicNumber matches
	if(vcb->magicNumber != MAGIC_NUMBER){ 
		
		//if the filesystem has not been initialized yet, initalize it
		printf ("Initializing File System with %ld blocks with a block size of %ld\n", totalBlocks, sizeOfBlock);
		
		//set the propertices for the vcb
		vcb->magicNumber = MAGIC_NUMBER;
		vcb->blockSize = sizeOfBlock;
		vcb->freeBlockCount = totalBlocks;
		
		//initialize the freespace management and rootDirectory
		int bitmapIndex = bitmapInit(totalBlocks, sizeOfBlock);
		int rootIndex = initDirectory(NULL, sizeOfBlock);
		
		//set the remaining properites of VCB
		vcb->bitmapIndex = bitmapIndex;
		vcb->rootIndex = rootIndex;
		vcb->numBlocks = totalBlocks;
		
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
