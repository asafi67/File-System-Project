/**************************************************************
 * Class:  CSC-415-01 Summer 2023
 * Names:Anish Khadka, Joe Sand, Ameen Safi
 * Student IDs:921952002,920525382, 920689065
 * GitHub Name:
 * Group Name: File System Soldiers
 * Project: Basic File System
 *
 * File: rootDir.h
 *
 * Description: header file for the rootDir.c
 *
 *
 *
 **************************************************************/
#ifndef ROOTDIR_H_
#define ROOTDIR_H_
#define EXTENT_COUNT 10
#define BUFFER_SIZE 50
#include <time.h>

#include "extent.h"

// definition of DE (directory entry) structure
typedef struct DE
{

    char name[20];                // array to hold the entry's name
    unsigned long int size;       // holds the entry's size
    int loc;                      // Int representing the entry's location
    time_t last_accessed;         // timestamp indicating the last time the entry accessed
    time_t created_at;            // timestamp indicating when the entry was created
    time_t modified_at;           // timestamp indicating the last time was modified
    char file_type;               // character representing the typoe of file
    extent extents[EXTENT_COUNT]; // an array of extents, max of 8
    int isDirectory;              // flag to identify whether the entry is a directory or file
} DE;

// function declaration for initializing a directory with a given block size
int initDir(DE *parent, int blockSize);

#endif