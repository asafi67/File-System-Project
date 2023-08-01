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
#ifndef BITMAP_H
#define BITMAP_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "volumeCB.h"
#include "fsLow.h"

// configuration constants, manually modify
#define BITMAP_POSITION 1
#define BITS_PER_BYTE 8

extern unsigned char *bitmap; // define a bitmap of char strings
extern int blockSize;         // declare variable for blocksize

// Function prototypes
int bitmapInit(int numBlocks, int blockSize);
char *byteToBinaryString(unsigned char inputByte);
unsigned char generateBitMask(int position);
int checkBitStatus(int bitPosition, char charToTest);
int findFreeBitIndex(int blocksNeeded, unsigned char *bitmapArray, int bitmapSize);
int allocateBlocks(int blocksRequired, unsigned char *bitmapArray, int bitmapSize);
void releaseBlocks(int blocksToRelease, int initialPosition);
int loadBitmap(void);

#endif // BITMAP_H