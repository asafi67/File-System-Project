#ifndef _VCB_H_
#define _VCB_H_

typedef struct {

    unsigned int magicNumber;
    
    unsigned int numOfBlocks;
    unsigned int blockSize;
    unsigned int freeBlocks;
    unsigned int allocatedBlocks;
} VolumeControlBlock;

extern VolumeControlBlock* vcb;

#endif

