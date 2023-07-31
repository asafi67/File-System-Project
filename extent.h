
#include "rootDir.h"


typedef struct {

    int count; //number of contihuous blocks in this extent
    int blockNumber; //Location of the first block of thje extent on disk

}Extent;