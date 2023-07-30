#ifndef _BIT_MAP_H
#define _BIT_MAP_H




extern unsigned char * bitmap;
extern int bitmapBlockSize;
int bitmapInit(int totalBlocks, int blockSize);
int allocateBlocks(int numBlocksToAllocate, unsigned char* bitmap, int bitmapSize);
void deallocateBlocks(int numBlocksToFree, int startingPos);
//loads the bitmap into memory if NOEXISTS returns 1 on success -1 on failure
int loadBitmap();


#endif
