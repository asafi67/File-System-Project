#ifndef ROOTDIR_H_
#define ROOTDIR_H_
#define EXTENT_COUNT 8
#define BUFFER_SIZE 50
#include <time.h>

#include "extent.h"



typedef struct DE{

    //this character array will store the name of an entry
    //entry name is limited to 20 characters
    char name[20];
    //size of an entry will be stored as a long
    //limited to 10mb
    unsigned long int size;
    //location of an entry is stored as an int
    int loc;
    //variable to note when entry was last accessed
    time_t last_accessed;
    //variable to note when entry was created
    time_t created_at;
    //variable to note when an entry was last modified
    time_t modified_at;
    //we use a char to note type of a file
     char file_type;
    //an array of extents which will have up to 8 extents
    extent extents[EXTENT_COUNT];
    //if directory entry points to folder then isDirectory = 1
    //if directory entry points to file then isDirectory = 0
    int isDirectory;
} DE;

int initDir(DE* parent, int blockSize);


#endif