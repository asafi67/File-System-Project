/**************************************************************
 * Class:  CSC-415-01 Summer 2023
 * Names:Anish Khadka, Joe Sand, Ameen Safi
 * Student IDs:921952002,920525382, 920689065
 * GitHub Name: asafi67
 * Group Name: File System Soldiers
 * Project: Basic File System
 *
 * File: volumeCB.h
 *
 * Description: Structure for the Volume Control Block.
 *
 * 
 *
 **************************************************************/
#ifndef _VCB_H_
#define _VCB_H_

typedef struct VolumeControlBlock
{
	// This is the signature of our file system
	// Checked to see if fs is mounted
	int magic_number;
	// Total number of blocks partitioned for the fs
	int num_blocks;
	// Size of a single block on disk for this fs
	int block_size;
	// Number of remaining free blocks in the partitioned region of the disk
	int freeBlockCount;
	// Beginning of bitmap on disk
	int bitmap_index;
	// Beginning of root on disk
	int root_index;
} VolumeControlBlock;

extern VolumeControlBlock *vcb;

#endif
