#ifndef _VCB_H_
#define _VCB_H_

typedef struct {

    unsigned int magicNumber; 
    
    unsigned int numOfBlocks; //# of blocks partitioned for fs
    unsigned int blockSize; //size of a single block in the fs
    unsigned int freeBlocks; //# of free blocks left within partitioning of disk
    unsigned int allocatedBlocks; //# of nonfree blocks in the partitioned region of disk
    unsigned int bitmap_index; // start of bitmap on disk
    unsigned int root_index;
} VolumeControlBlock;

extern VolumeControlBlock* vcb;

#endif

