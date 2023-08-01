/**************************************************************
 * Class:  CSC-415-01 Summer 2023
 * Names:Anish Khadka, Joe Sand, Ameen Safi
 * Student IDs:921952002,920525382, 920689065
 * GitHub Name: asafi67
 * Group Name: File System Soldiers
 * Project: Basic File System
 *
 * File: rootDir.c
 *
 * Description: implementation for managing and initalizing a 
 * directory within a custom file system. 
 * 
 *
 **************************************************************/
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

// Function to initialize a new directory on the disk;
// returns the first block if successful or -1 if failed.
int initDir(DE *parent, int blockSize)
{
    int bit_byte_size = (vcb->num_blocks + 7) / 8;                    // calculate the number of bytes required
    DE *directory = malloc(BUFFER_SIZE * sizeof(DE) + blockSize - 1); // memory allocation

    // check if memory allocation failed
    if (directory == NULL)
    {
        printf("Memory allocation failure in initDir\n");
        return -1;
    }

    // loop and initalize each directory entry to a free state
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        directory[i].name[0] = '\0'; // set the name of every directory item to empty string (free)
    }

    // Load the bitmap, return failure if unable to do so.
    if (loadBitmap() == -1)
    {
        printf("Bitmap loading failure in initDir\n");
        return -1;
    }

    // Request space on disk using the bitmap's allocateBlocks function.
    int block_number = allocateBlocks((BUFFER_SIZE * sizeof(DE) + blockSize - 1) / blockSize, bitmap, bit_byte_size);

    // congigure the current directory entry, '.'
    DE direc;
    direc.name[0] = '.';
    direc.name[1] = '\0';
    direc.size = BUFFER_SIZE * sizeof(DE) + blockSize - 1;
    direc.loc = block_number;
    direc.isDirectory = 1;
    directory[0] = direc;

    // check wheter there is a parent directory if not, set '..' to point to iteslf
    if (parent == NULL)
    {
        directory[1] = direc;
        directory[1].name[0] = '.';
        directory[1].name[1] = '.';
        directory[1].name[2] = '\0';
    }
    else
    {
        // else set '..' to point to the parent directory
        directory[1] = parent[0];
        directory[1].name[0] = '.';
        directory[1].name[1] = '.';
        directory[1].name[2] = '\0';
    }

    int dirNumBlocks = (BUFFER_SIZE * sizeof(DE) + blockSize - 1) / blockSize;

    // write the directory entries to disk at the specified block number
    if (LBAwrite(directory, dirNumBlocks, block_number) != dirNumBlocks)
    {
        printf("Failed to perform LBAwrite within initDir\n");
        return -1;
    }

    // return the block number of the new directory
    return block_number;
}
