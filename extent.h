
#include "rootDir.h"


typedef struct {
    //number of contiguous blocks in extent
    int count;
    //the location of the first block of the extent on disk
    //if the blockNumber is negative, this indicates that it is Not
    //an extent but a pointer to an indirect extent table.
    int blockNumber;

}Extent;