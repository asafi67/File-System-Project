#ifndef ROOTDIR_H_
#define ROOTDIR_H_
#define EXTENT_COUNT 8
#define BUFFER_SIZE 50
#include <time.h>

#include "extent.h"



typedef struct DirectoryEntry{
//TODO: copy all of the fields we submitted for our first group project in here
    /* name of the entry, limit to 20 characters */
    char name[20];
    /* size of the entry in bytes, 10mb limit */
    unsigned long int size;
    /* location of the entry corresponds to sector number */
    int location;
    /* all date ints are stored like mmddyyyy */
    /* the last time the file was accessed */
    time_t dateLastAccessed;
    /* the date of creation of the file */
    time_t dateCreated;
    /* last time the file was modified */
    time_t dateLastModified;
    /* file extension/type */
    unsigned char fileType;
    //extent array that can hold a max of 8 extents
    Extent extents[EXTENT_COUNT];
    //if directory entry points to folder
    //isDir = 1
    //if directory entry points to file
    //isDir = 0
    int isDir;
} DirectoryEntry;

int initDirectory(DirectoryEntry* parent, int blockSize);


#endif