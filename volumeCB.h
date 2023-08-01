#ifndef _VCB_H_
#define _VCB_H_



typedef struct VolumeControlBlock {
	//this is the signature of our file system 
	//checked to see if fs is mounted
    int magic_number;
	//total number of blocks partitioned for the fs
	int num_blocks;
	//size of a single block on disk for this fs
	int block_size;
	//number of remaining free blocks in the partitioned region of the disk
	int freeBlockCount;
	//beginning of bitmap on disk
	int bitmap_index;
	//beginning of root on disk
	int root_index;
} VolumeControlBlock;

extern VolumeControlBlock* vcb;

#endif

