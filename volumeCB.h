#ifndef _VCB_H_
#define _VCB_H_



typedef struct VCB {
	//the signature of the fs 
	//checked to see if fs is mounted
    int magicNumber;
	//starting block of bitmap on disk
	int bitmapIndex;
	//starting block of root on disk
	int rootIndex;
	//total number of blocks partitioned for the fs
	int numBlocks;
	//size of one block on disk for this fs
	int blockSize;
	//number of remaining free blocks in the partitioned region of the disk
	int freeBlockCount;
} VCB;

extern VCB* vcb;

#endif

