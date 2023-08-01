/**************************************************************
 * Class:  CSC-415-01 Summer 2023
 * Names:Anish Khadka, Joe Sand, Ameen Safi
 * Student IDs:921952002,920525382, 920689065
 * GitHub Name:
 * Group Name: File System Soldiers
 * Project: Basic File System
 *
 * File: fsInit.c
 *
 * Description: Main driver for file system assignment.
 *
 * This file is where you will start and initialize your system
 *
 **************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "volumeCB.h"
#include "bitmap.h"
#include "fsLow.h"

// configuration constants, manually modify
#define BITMAP_POSITION 1
#define BITS_PER_BYTE 8

unsigned char *bitmap; // define a bitmap of char strings
int blockSize;         // declare variable for blocksize

// returns the address of the bitmap
// accepts the # of total blocks and the block size
int bitmapInit(int numBlocks, int blockSize)
{

    // Initialize the Free Space
    int bytesRequired = ((numBlocks + 7) / 8);
    int blocksRequired = (bytesRequired + blockSize - 1) / blockSize;
    blockSize = blocksRequired;

    // Before actually initializng the bits, we must allocate the necessary
    // memory, utilizing calloc, calloc sets all elements to 0.
    bitmap = calloc((blocksRequired * blockSize), sizeof(unsigned char));

    // error check, check if bitmap failed to initalize
    if (bitmap == NULL)
    {
        printf("failed to allocate memory for the bitmap in the bitmapInit function \n");
        return -1;
    }

    // mark the first bit for the Volume Control Block
    // mark bits for bitmap starting at the defined variable BITMAP_POSITION
    int allocationStatus = allocateBlocks((1 + blocksRequired), bitmap,
                                          (blocksRequired * blockSize));

    // Check if the allocation was successful.
    // A return value of -1 indicates failure.
    if (allocationStatus == -1)
    {
        printf("Unable to locate free space for the VCB and bitmap\n");
        printf("Number of blocks = %d\n", blocksRequired);
        return -1;
    }

    // Write the allocated bitmap to disk at the specified position using LBAwrite.
    LBAwrite(bitmap, blocksRequired, BITMAP_POSITION);

    // Print the size of the bitmap in blocks to the console
    printf("Size of bitmap in blocks: %d\n", blocksRequired);

    // Return the position where the bitmap was written
    return BITMAP_POSITION;
}

// Function in order to convert an unsigned char (byte) to its binary string
char *byteToBinaryString(unsigned char inputByte)
{

    // Initalize static array to hold binary representation;
    // +1 for null terminator, initialized to zeros
    static char binaryString[BITS_PER_BYTE + 1] = {0};

    // Variable for position within the binary representation array
    int position;

    // Loop through the bits of the byte, starting from the most significant bit
    // and ending with the least significant bit
    for (position = BITS_PER_BYTE - 1; position >= 0; position--)
    {

        // Determine the current bit (0 or 1) and convert to character '0' or '1'
        binaryString[position] = (inputByte % 2) + '0';
        // Right-shift the bits of the input byte by 1 (divide by 2) to process the next bit
        inputByte /= 2;
    }

    // retrun the binary string representation of the input byte
    return binaryString;
}

// Function to create a bitmask for a given postion
// 'position' specifies the bit position within the byte
unsigned char generateBitMask(int position)
{
    // start with a mask of 00000001
    unsigned char bitMask = 1;
    // Shift the '1' to the specified position within the byte
    bitMask = bitMask << position;
    // return the new bitMask
    return bitMask;
}

// Function to check if a specific bit is set within a character
// 'bitPosition' ranges from 0 (rightmost) to 7 (leftmost)
int checkBitStatus(int bitPosition, char charToTest)
{

    // Create a mask for the specified bit position
    unsigned char mask = generateBitMask(7 - bitPosition);
    // Initialize variable to track if bit is set
    int isBitSet = 0;
    // check if the bit is set at the specified position
    if ((mask & charToTest) == mask)
    {
        isBitSet = 1; // If the bit is set, update 'isBitSet' to 1
    }
    // Return 1 is the bit is set, 0 if not
    return isBitSet;
}

// Function to find a contiguous block of free bits within a bitmap
// 'blocksNeeded' is the number of contiguous free bits required
// 'bitmapArray' is the pointer to the bitmap
// 'bitmapSize' is the size of the bitmap in bytes
int findFreeBitIndex(int blocksNeeded, unsigned char *bitmapArray, int bitmapSize)
{ // returns the index of the bit not byte

    int firstFreeBit;
    int lastFreeBit;
    int currentBit;

    // Loop to iterate through all the bits in the bitmap
    for (currentBit = 0; currentBit < bitmapSize * 8; currentBit++)
    {
        // Calculate the index of the current byte
        int byteIndex = currentBit / 8;
        // Calculate the offeset of the current bit within byte
        int bitOffset = currentBit % 8;

        // Check if the current bit is not set
        if (checkBitStatus(bitOffset, bitmapArray[byteIndex]) == 0)
        {
            // if its free
            firstFreeBit = currentBit;
            lastFreeBit = currentBit + blocksNeeded;

            // Iterate through the required contiguous bits to ensure all are free
            for (; currentBit < lastFreeBit; currentBit++)
            {
                byteIndex = currentBit / 8;
                bitOffset = currentBit % 8;

                // check if any bit within this range is not free
                if (checkBitStatus(bitOffset, bitmapArray[byteIndex]) == 1)
                {
                    break; // if not free break out of loop
                }
            }

            // check if all required contiguous bits are free
            if (currentBit == lastFreeBit)
            {
                // return the index of the first free bit
                return firstFreeBit;
            }
        }
    }
    // error messages for no suitable block found
    printf("Parameter blocksNeeded: %d\n", blocksNeeded);
    printf("Could not find start index in findFreeBitIndex\n");
    return -1; // we do not have room
}
// function to load the bitmap into memory if it does not exist
// should return 1 on success, -1 on failure
int loadBitmap()
{
    // check if bitmap is already in memory
    if (bitmap == NULL)
    {

        // Calculate the size of the bitmap in bytes
        int bitmapByteSize = (((vcb->num_blocks + 7) / 8) + (vcb->block_size - 1)) / vcb->block_size;

        // If the bitmap is not in memory, allocate space for it
        bitmap = malloc(bitmapByteSize + vcb->block_size - 1);

        // Check if memory allocation failed
        if (bitmap == NULL)
        {
            printf("Memory allocation failed for bitmap in loadBitmap()\n");
            return -1;
        }
        // Calculate the number of blocks needed to hold the bitmap
        int numberOfBitmapBlocks = (bitmapByteSize + vcb->block_size - 1) / vcb->block_size;

        // Read the bitmap from the disk into memory
        if (LBAread(bitmap, numberOfBitmapBlocks, vcb->bitmap_index) != numberOfBitmapBlocks)
        {
            printf("Disk read failed to load bitmap into memory\n");
            return -1;
        }
    }
    return 1;
}
// function to allocate a specific number of contiguous blocks in a bitmap
int allocateBlocks(int blocksRequired, unsigned char *bitmapArray, int bitmapSize)
{
    // handle error if VCB is NULL
    if (vcb == NULL)
    {
        printf("memory allocation for vcb failed!\n");
        return -1;
    }
    // handle error is there's not enough free blocks on the disk
    if (vcb->freeBlockCount < blocksRequired)
    {
        printf("Not enough space on disk to allocate %d blocks\nAvailable blocks: %d\n",
               blocksRequired, vcb->freeBlockCount);
        return -1;
    }

    // Find the starting index of freee contiguous blocks in the bitmap
    int startingIndex = findFreeBitIndex(blocksRequired, bitmapArray, bitmapSize);

    // handle error if no space is available in the bitmap
    if (startingIndex == -1)
    {
        return -1;
    }

    // Loop through the required number of contiguous blocks and mark them as allocated
    for (int currentBit = startingIndex; currentBit < blocksRequired + startingIndex;
         currentBit++)
    {

        int byteIndex = currentBit / 8; // Calculate the index of the current byte
        int bitOffset = currentBit % 8; // Calculate the offset of the current bit within the byte

        // Create a mask for the current bit
        unsigned char bitMask = generateBitMask(7 - bitOffset);

        // Mark the current bit as allocated (set to 1)
        bitmapArray[byteIndex] = (bitmapArray[byteIndex] | bitMask);
    }

    // Update the number of available blocks in the Volume Control Block
    vcb->freeBlockCount -= blocksRequired;

    // Write the updated VCB back to disk (block number 0)
    LBAwrite(vcb, 1, 0);

    // Write the updated bitmap to disk
    LBAwrite(bitmapArray, blockSize, BITMAP_POSITION);

    // Return the starting index of the allocated blocks
    return startingIndex;
}

// Function to deallocate a specific number of contiguous blocks in a bitmap
// 'blocksToRelease' is the number of contiguous blocks to free
// 'initialPosition' is the starting bit position of the blocks to be freed
void releaseBlocks(int blocksToRelease, int initialPosition)
{

    // check if you can load the bitmap
    if (loadBitmap() == -1)
    {
        printf("Bitmap loading failed in relasaeBlocks function\n");
        return;
    }

    // Calculate the starting byte index in the bitmap array
    int byteIndexStart = initialPosition / 8;

    // Calculate the starting bit's offset within the byte
    short bitOffsetStart = initialPosition % 8;

    // Create a mask to target the specific bit for deallocation
    unsigned char bitMask = generateBitMask(7 - bitOffsetStart);

    while (blocksToRelease > 0)
    {
        // Clear the current bit (set to 0), marking it as free
        bitmap[byteIndexStart] &= (~bitMask);

        initialPosition++; // Move to the next bit
        blocksToRelease--; // Decrement the count of blocks to be freed

        // Recalculate the current byte and bit positions in the array
        byteIndexStart = initialPosition / 8;
        bitOffsetStart = initialPosition % 8;

        // Update the mask to target the next bit
        bitMask = generateBitMask(7 - bitOffsetStart);
    }
}
