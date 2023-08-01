#ifndef EXTENT_H
#define EXTENT_H
#include "rootDir.h"


typedef struct {

    //the location of the first block of the extent on disk
    int block_number;
    //number of contiguous blocks in an extent
    int contiguous_blocks;
  
}extent;


#endif