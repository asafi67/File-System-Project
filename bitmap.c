#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>


#include "volumeCB.h"
#include "bitmap.h"
#include "fsLow.h"



#define BITMAP_LOCATION 1
//defined to build .o, please chnage
#define CHAR_BIT 1
//declare a bitmap blocksize
int bitmapBlockSize;
//declare bitmap
unsigned char* bitmap;

//returns the address of the bitmap
int bitmapInit(int totalBlocks, int blockSize){    //totalBlocks is the total number of blocks for the file system.  

// Initialize the Free Space
// a. Let’s assume you are using a bitmap for your free space management. Given the 
// default size, you will have 19,531 blocks – so you need 19,531 bits or 2,442 bytes 
// or 5 blocks (so from this alone you can see that the free space map is NOT part of 
// the VCB, but rather the VCB would have the block number where your free space 
// starts).
  int numBytes = ( ( totalBlocks + 7) / 8 );    
  int numBlocks = (numBytes + blockSize -1)  / blockSize;
  bitmapBlockSize = numBlocks;
// b. Before you can initialize the bits, you must have the memory to do so. So malloc 
// the space needed. In this example 5 * blockSize
//calloc sets all elements to 0.
   bitmap =  calloc((numBlocks * blockSize), sizeof(unsigned char));
  if(bitmap == NULL){
    printf("ERROR occurred in malloc for bitmap in bitmap_init function\n");
    return -1;
  }

//set the first bit to a 1 for the VCB
//set numBlocks bits to 1 for bitmap starting at BITMAP_LOCATION
int res = allocateBlocks((1+numBlocks), bitmap, (numBlocks*blockSize)); //1 for the vcb + numBlocks for the bitmap as first arg. numBlocks*blockSize for bitmapSize
if(res == -1){
  printf("failed to find free space for the VCB and the bitmap.\n");
  printf("numb blocks = %d\n",numBlocks);
  return -1;
}
// d. Now write that to disk with LBAwrite(theFreeSpaceMap, 5, 1) – that is write 5 
LBAwrite(bitmap, numBlocks, BITMAP_LOCATION);
//TODO: check what the return value means for LBAwrite and error handle it
printf("size of bitmap in blocks: %d\n", numBlocks);
return BITMAP_LOCATION; 
}


char *charToBinary ( unsigned char c )
{
    static char binary[CHAR_BIT + 1] = {0};
    int i;
    for( i = CHAR_BIT - 1; i >= 0; i-- )
    {
        binary[i] = (c % 2) + '0';
        c /= 2;
    }
   return binary;
}

unsigned char createMask(int x){ //returns a char bitmask representation of the input int
 unsigned char mask = 1;   //TODO check if this works for deallocateBlocks too. might need to set mask to 0 sometimes.
    mask = mask << x;
 return mask;
}

int isDigitFree(int digit, char test){ //from right to left so if you want to check leftmost bit you input 7 and if you want the rightmost bit you do 0
 unsigned char mask = createMask(7-digit); 
 int result = 0; 
 if((mask & test) == mask){
  result = 1;   
 }
    return result;
} //returns 1 if it is not free and 0 if it is free


//bitmapSize is the number of bytes allocated for bitmap third param is in bytes
int getStartIndex(int numBlocksToAllocate, unsigned char* bitmap, int bitmapSize){ //returns the index of the bit not byte 
 //1 find the first zero in bitmap  
//2. check next numBlocksToAllocate amount of bits
//if a 1 is found then start over at step 1 at the bit after the one 
    int startBit; //to be returned 
    int endBit;  
    int currBit;
    for(currBit =0; currBit < bitmapSize*8; currBit++){
        int byte = currBit / 8; //the byte in the bitmap that we are in      
        int offset = currBit % 8;  //the bit in the byte that we are in
        if(isDigitFree(offset, bitmap[byte]) == 0){ //if it is free
            startBit = currBit; 
             endBit = currBit + numBlocksToAllocate; 
             //this for loop is probably unnecessary
            for(; currBit < endBit; currBit++){ //check numBlocksToAllocate number of contiguous bits from currBit 
                 byte = currBit / 8;
                offset = currBit % 8;
                 if(isDigitFree(offset, bitmap[byte]) == 1){ //if it is not free  
                    break;
                 }
            }
            if(currBit == endBit){
                return startBit;
            }
        }
    }
    printf("parameter numblockstoallocate: %d\n", numBlocksToAllocate);
    printf("failed to find start index in getStartIndex\n");
    return -1; //we do not have room
}
 //flips numBlocksToAllocate number of free contiguous bits to 1s
int allocateBlocks(int numBlocksToAllocate, unsigned char* bitmap, int bitmapSize){ //returns the starting block number of the free space allocated on disc
    //get vcb for bitmap and freeblockcount
    //VCB* vcb = malloc(sizeof(VCB));
    if(vcb==NULL){
        printf("malloc for vcb failed!\n");
        return -1;
    }
    //LBAread(vcb, 1, 0); //vcb is at block zero
    if(vcb->freeBlockCount < numBlocksToAllocate){
        printf("not enough space on disc to allocate %d number of blocks\n free block count is %d\n", numBlocksToAllocate, vcb->freeBlockCount);
        return -1;
    }
    //find the index to start filling in 1s
    int startIndex = getStartIndex(numBlocksToAllocate, bitmap, bitmapSize);
   
    if(startIndex == -1){ //if there is no space in bitmap 
        return -1; 
        
    }
    //fill in numBlocksToAllocate number of 1s
    for(int currBit = startIndex; currBit< numBlocksToAllocate+startIndex; currBit++){
        int byte = currBit / 8; //the byte in the bitmap that we are in  
        int offset = currBit % 8; //the bit in the byte that we are at
        unsigned char mask =   createMask(7-offset); //rightmost bit - offset
        bitmap[byte] = ( bitmap[byte] |  mask ); //set the bit to a 1. Set it to a zero with & instead of |
    }
    //update number of free blocks in VCB
    vcb->freeBlockCount -= numBlocksToAllocate;
    //write the vcb back to disc
    LBAwrite(vcb, 1, 0); //vcb is at block number 0
    //return the starting index
    LBAwrite(bitmap,bitmapBlockSize,BITMAP_LOCATION);
    return startIndex;
}


//TODO: research extents
//function may need to accept extents as an array arg of type “extent”
//also research indirect extent table…
//returns a 1 upon success and a 0 upon failure
//flips numBlocksToFree number of contiguous bits from startingPos (including //startingPos)
void deallocateBlocks(int numBlocksToFree, int startingPos){
   //load bitmap if not already in memory
   if(loadBitmap() == -1){
    printf("loadBitmap failed in deallocatBlocks\n");
    return;
   }
    //calculate the starting byte AKA what index to start at in the bitmap array
	int startingByteAddress = startingPos/8;
    //calculate the bit in the byte (ie offset in the array element)
	short startingBitAddress = startingPos % 8;
    //create mask to flip bits to off
unsigned char mask = createMask(7-startingBitAddress);  
	while(numBlocksToFree > 0){
		//set the current bit to a zero.
		bitmap[startingByteAddress] &=  (~mask); 
        //move forward one bit
        startingPos++;
        //just freed a block so decrement numBlocksToFree
        numBlocksToFree--; 
         //recalculate the position in the array
        startingByteAddress=startingPos/8;
        startingBitAddress = startingPos % 8;
        //update mask to target the next bit
        mask = createMask(7-startingBitAddress);  
    }

}
//loads the bitmap into memory if NOEXISTS returns 1 on success -1 on failure
int loadBitmap(){
    //check if bitmap is in memory first
if(bitmap==NULL){
    //number of bytes that the bitmap is
     int bitByteSize = ( ( ( vcb->num_blocks + 7/8) / 8 ) + (vcb->block_size -1) / vcb->block_size)  / vcb->block_size;
    //bitmap is not yet in memory so load it 
    bitmap = malloc(bitByteSize + vcb->block_size-1);
    if(bitmap==NULL){
        printf("malloc failed for bitmap in loadBitmap function\n");
        return -1;
    }
    int bitmapBlocks = (bitByteSize + vcb->block_size - 1) / vcb->block_size;
    if(LBAread(bitmap, bitmapBlocks, vcb->bitmap_index) != bitmapBlocks){
        printf("LBAread failed to load bitmap from disk\n");
        return -1;
    }
}
return 1;
}