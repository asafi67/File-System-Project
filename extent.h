/**************************************************************
 * Class:  CSC-415-01 Summer 2023
 * Names:Anish Khadka, Joe Sand, Ameen Safi
 * Student IDs:921952002,920525382, 920689065
 * GitHub Name: asafi67
 * Group Name: File System Soldiers
 * Project: Basic File System
 *
 * File: extent.h
 *
 * Description: This header file defines a structure named 'extent'
 * that represents a contiguous set of blocks in a file system. An
 * extent is used as a continuous area of storage used to represent
 * part of a file, improving storage efficency and access speed.
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