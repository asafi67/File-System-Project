#ifndef _BIT_MAP_H
#define _BIT_MAP_H


extern char* bitmap;
// Structure for free space block allocation
typedef struct {
    int start;
    int count;
} blockAllocation;
void releaseBlocks(int start, int count, int bytesPerBlock);
int loadFreeSpace(int blockCount, int bytesPerBlock);
int initFreeSpace(int blockCount, int bytesPerBlock);
blockAllocation allocateBlocks (int required, int minPerExtent);

#endif