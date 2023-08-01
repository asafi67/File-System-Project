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
#ifndef EXTENT_H
#define EXTENT_H
#include "rootDir.h"

typedef struct
{

    // the location of the first block of the extent on disk
    int block_number;
    // number of contiguous blocks in an extent
    int contiguous_blocks;

} extent;

#endif