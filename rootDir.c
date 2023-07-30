
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "volumeCB.h"
#include "bitmap.h"
#include "fsLow.h"
#include "rootDir.h"
#include "mfs.h"


unsigned char* bitmap;

//returns first block of new dir on disk on success and -1 on failure
int initDirectory(DirectoryEntry* parent, int blockSize){
//number of bytes in bitmap
int bitByteSize = ( vcb->numBlocks + 7) / 8; 
// allocate mem for the new directory array
DirectoryEntry* dir = malloc(BUFFER_SIZE * sizeof(DirectoryEntry) + blockSize-1);

if(dir==NULL){
    printf("malloc failed in initDirectory\n");
    return -1;
}
// put "." and ".." directoryEntry instances into the buffer. at index 0 and 1 respectively.
// initialize the "." DirectoryEntry to itself.

//initialize all directory items to a known free state
for (int i = 0; i < BUFFER_SIZE; i++){
    //set the name of every directory item to NULL
    dir[i].name[0] = '\0';
}

if(loadBitmap() == -1){
    printf("loadBitmap function failed in initDirectory function \n");
    return -1;
}

// allocate space on disk (via the bitmap's allocateBlocks function) and set blockNumber to the returned value.
 int blockNumber = allocateBlocks(( BUFFER_SIZE * sizeof(DirectoryEntry) + blockSize-1) / blockSize , bitmap, bitByteSize);

DirectoryEntry direc;
//initialize directory
direc.name[0] = '.';
direc.name[1] = '\0';
direc.size = BUFFER_SIZE * sizeof(DirectoryEntry) + blockSize-1;
direc.location = blockNumber;
direc.isDir = 1;
dir[0] = direc;

// If parent == NULL then initialize the ".." entry to pointer to itself. Else: initialize the ".." entry to parent.
if(parent == NULL){
    dir[1] = direc;
    dir[1].name[0] = '.';
    dir[1].name[1] = '.';
    dir[1].name[2] = '\0';
}
else{
    dir[1] = parent[0];
    dir[1].name[0] = '.';
    dir[1].name[1] = '.';
    dir[1].name[2] = '\0'; 
}

int dirNumBlocks = (BUFFER_SIZE * sizeof(DirectoryEntry) + blockSize-1) / blockSize;
// write the buffer to the disk at the block number. 

if(LBAwrite(dir, dirNumBlocks, blockNumber) != dirNumBlocks){
    printf("error occurred in LBAwrite in initDirectory function\n");
    return -1;
}


return blockNumber;
}