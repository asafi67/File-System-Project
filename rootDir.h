#ifndef ROOTDIR_H_
#define ROOTDIR_H_
#define EXTENT_COUNT 8
#define BUFFER_SIZE 50
#include <time.h>

#include "extent.h"

// Structure for directory entry
typedef struct {
    char name[256]; //the name of an entry is limited to 256 characters
    unsigned long int size;
    unsigned int loc;
    time_t lastAccessedDate;
    time_t createdAt;
    time_t modifiedAt;
    extent extents[EXTENT_COUNT];
    int isDirectory;
} DE;

int initDir(int minEntries, DE* parent);

#endif