





typedef struct {
    unsigned int numOfBlocks;
    unsigned int blockSize;
    unsigned int freeBlocks;
    unsigned int allocatedBlocks;
} VolumeControlBlock;

extern VolumeControlBlock* vcb;

