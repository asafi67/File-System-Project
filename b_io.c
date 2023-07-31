/**************************************************************
 * Class:  CSC-415-0# Fall 2021
 * Names:
 * Student IDs:
 * GitHub Name:
 * Group Name:
 * Project: Basic File System
 *
 * File: b_io.c
 *
 * Description: Basic File System - Key File I/O Operations
 *
 **************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "rootDir.h"
#include "bitmap.h"
#include "volumeCB.h"
#include "mfs.h"
#include "extent.h"
#include "fsLow.h"


#include <math.h>

#define MAXFCBS 20
#define B_CHUNK_SIZE 512
#define FILE_INITIAL_NUM_BLOCKS 10



unsigned char* bitmap;
typedef struct b_fcb
{
	/** TODO add al the information you need in the file control block **/
	char *buf;	// holds the open file buffer
	int index;	// holds the current position in the buffer
	int buflen; // holds how many valid bytes are in the buffer

	int current_block;	 // holds the current block #
	int num_blocks;		 // holds # of blocks a file habits
	int bytes_read;		 // holds # of bytes which has been read
	int current_extent;	 // holds int specifying the extent we are on
	int file_offset;	 // byte we are on in file. Can be altered in b_seek
	int permissions;	 // depends on b_open. notes type of access
	int parent_location; // location of the parent array for file
	DE *file_entry;		 // a file's entry in the directory

} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0; // Indicates that this has not been initialized

// Method to initialize our file system
void b_init()
{
	// init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
	{
		fcbArray[i].buf = NULL; // indicates a free fcbArray
	}

	startup = 1;
}

// Method to get a free FCB element
b_io_fd b_getFCB()
{
	for (int i = 0; i < MAXFCBS; i++)
	{
		if (fcbArray[i].buf == NULL)
		{
			return i; // Not thread safe (But do not worry about it for this assignment)
		}
	}
	return (-1); // all in use
}

// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char *filename, int flags)
{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//

	if (startup == 0)
		b_init(); // Initialize our system

	returnFd = b_getFCB(); // get our own file descriptor
						   // check for error - all used FCB's
	// as ParsePath mutates, create copy of filename
	char *pathCopy = malloc(strlen(filename) + 1);
	strcpy(pathCopy, filename);

	// declare a PatPassReturn object and parse the path
	PathReturn parent = parsePath(filename);
	if (parent.status_code == 0)
	{
		// path is invalid
		printf("file not found\n");
		return -1;
	}

	DE fileEntry;
	fileEntry.name[0] = '\0';
	// path is valid and last element does not exist
	if (parent.status_code == 1)
	{ // home/Desktop/newFile.txt
		if ((flags & O_CREAT) == O_CREAT)
		{

			// the path does not exist yet so create the file
			// get the name of the new file to be created

			// get starting index of last element in path
			// Ex.:If path a home/Desktop/newFolder
			// We need "newFolder"
			int index = 0;
			for (int i = strlen(pathCopy) - 1; i >= 0; i--)
			{
				if (pathCopy[i] == '/')
				{
					index = ++i;
					break;
				}
			}

			char newName[strlen(pathCopy) - index + 1];
			int newNameSize = strlen(pathCopy) - index;
			// copy over the chars starting at index into newName
			int i = 0;
			while (pathCopy[index] != '\0' && i < newNameSize)
			{
				newName[i] = pathCopy[index];
				i++;
				index++;
			}
			// null terminate the string
			newName[i] = '\0';

			fileEntry = fs_mkfile(parent.direc, newName, newNameSize);

			if (fileEntry.name[0] == '\0')
			{
				printf("failed to make file.\n");
				return -1;
			}
		}
		else
		{
			// create flag is not set so file cannot be created
			printf("File not found.\n");
			return -1;
		}
	}
	else
	{
		// path is valid and last element does exist
		if (parent.direc[parent.index_last].isDirectory == 1)
		{
			// user tried to open a directory instead of a file
			printf("cannot open directory with this command.\n");
			return -1;
		}
		// last element exists and is a file
		// assign the file to the last path element
		fileEntry = parent.direc[parent.index_last];
	}
	// allocate memory for open file buffer
	fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);

	// Initialize the int variables to 0
	fcbArray[returnFd].index = 0;
	fcbArray[returnFd].buflen = B_CHUNK_SIZE; // B_CHUNK_SIZE - index
	fcbArray[returnFd].current_block = 0;
	fcbArray[returnFd].bytes_read = 0;
	fcbArray[returnFd].file_offset = 0;
	fcbArray[returnFd].parent_location = parent.direc[0].loc;
	*fcbArray[returnFd].file_entry = fileEntry;

	// calculate the number of blocks that the file takes on disk
	int numberOfBlocks = 0;
	int i = 0;
	while (fcbArray[returnFd].file_entry->extents[i].contiguous_blocks > 0)
	{
		numberOfBlocks += fcbArray[returnFd].file_entry->extents[i].contiguous_blocks;
		++i;
	}
	printf("file location: %d and file number of blocks %d\n", fileEntry.loc, numberOfBlocks);
	fcbArray[returnFd].num_blocks = numberOfBlocks;
	fcbArray[returnFd].current_block = i;
	// set the file permissions based on the flags
	int permissions;
	if ((flags & O_RDONLY) == O_RDONLY)
	{
		permissions = 1;
	}
	if ((flags & O_WRONLY) == O_WRONLY)
	{
		permissions = 2;
	}
	if ((flags & O_RDWR) == O_RDWR)
	{
		permissions == 3;
	}
	if (((flags & O_TRUNC) == O_TRUNC) && (permissions != 1) && ((flags & O_CREAT) != O_CREAT))
	{
		fileEntry.size = 0;
		parent.direc[parent.index_last] = fileEntry;
		printf("truncating the file: %s\n", filename);
		// updated fileEntry so write it to disc
		int numBlocksForParent = (BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
		if (LBAwrite(parent.direc, numBlocksForParent, parent.direc[0].loc) != numBlocksForParent)
		{
			printf("LBAwrite failed in b_open. Did not truncate file.\n");
			return -1;
		}
		// also update the size of the file entry in the FCB
		fcbArray[returnFd].file_entry->size = 0;
	}
	printf("Opening file %s extentLocation %d and location is: %d\n", fcbArray[returnFd].file_entry->name, fcbArray[returnFd].file_entry->extents[0].block_number, fcbArray[returnFd].file_entry->loc);
	fcbArray[returnFd].permissions = permissions;
	return (returnFd); // all set
}

// Interface to seek function
int b_seek(b_io_fd fd, off_t offset, int whence)
{
	if (startup == 0)
		b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); // invalid file descriptor
	}

	if (whence == SEEK_SET)
	{
		fcbArray[fd].file_offset = offset;
		return offset;
	}
	if (whence == SEEK_CUR)
	{
		fcbArray[fd].file_offset += offset;
		return fcbArray[fd].file_offset;
	}
	if (whence == SEEK_END)
	{
		fcbArray[fd].file_offset = fcbArray[fd].file_entry->size + offset;
		return fcbArray[fd].file_offset;
	}

	return (-1); // Change this
}

// Interface to write function
int b_write(b_io_fd fd, char *buffer, int count)
{
	if (startup == 0)
		b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); // invalid file descriptor
	}
	if (fcbArray[fd].permissions == O_RDONLY)
	{
		printf("Do not have permission to write this file.\n");
		return -1;
	}

	// bytesWritten to be returned at end of function if nothing goes wrong
	int bytesWritten = count;

	// create a buffer for the first block
	char *startBlock = malloc(B_CHUNK_SIZE);
	if (startBlock == NULL)
	{
		printf("malloc failed in b_write\n");
		return -1;
	}

	int blockDest = fcbArray[fd].file_offset / vcb->block_size;
	int offset = fcbArray[fd].file_offset % vcb->block_size;

	// check if we have enough space in file to accommodate count number of bytes
	if ((fcbArray[fd].num_blocks * vcb->block_size - fcbArray[fd].file_offset) < count)
	{
		printf("not enough space in file. Expanding file...\n");
		// there is not enough space to fit count many bytes in the file
		// therefore, allocate more space for the file

		// calculate the number of blocks needed
		int bytesNeeded = count - (fcbArray[fd].num_blocks * vcb->block_size - fcbArray[fd].file_offset);
		// convert bytes to blocks
		int numBlocksNeeded = (bytesNeeded + vcb->block_size - 1) / vcb->block_size;
		// check if numBlocksNeeded is less than the default we would perscribe for the current extent
		extent nextExt;
		int i = 0;
		// locate the next extent to be initialized in the file's extents array.
		while (fcbArray[fd].file_entry->extents[i].contiguous_blocks != 0)
		{
			i++;
		}

		// formula to double the number of blocks of the previous extent for current extent
		int wouldPrescribe = 10 * (int)pow(2, i);
		numBlocksNeeded = (numBlocksNeeded > wouldPrescribe) ? numBlocksNeeded : wouldPrescribe;

		// Ask the bitmap for available space
		// first load the bitmap into memory if not already loaded
		if (loadBitmap() == -1)
		{
			printf("loadBitmap function failed in b_write function \n");
			free(startBlock);
			return -1;
		}
		int bitByteSize = (vcb->num_blocks + 7) / 8;
		// allocate space on disk (via the bitmap's allocateBlocks function) and set blockNumber to the returned value.
		int blockNumber = allocateBlocks(numBlocksNeeded, bitmap, bitByteSize);

		// 2. If it returns a negative number, then update count to the remaining number of bytes in the file and proceed. (partial write MANPAGE)
		if (blockNumber < 0)
		{
			// set count to the remaining number of bytes in file. PARTIAL WRITE
			printf("Not enough space on disk\n Executing PARTIAL READ.\n");
			count = fcbArray[fd].num_blocks * vcb->block_size - fcbArray[fd].file_offset;
			bytesWritten = count;
		}
		else
		{
			// If allocateBlocks found space for the new extent then create an extent for the file

			nextExt.contiguous_blocks = numBlocksNeeded;
			nextExt.block_number = blockNumber;
			fcbArray[fd].file_entry->extents[i] = nextExt;
			// merge extents should they occur contiguously on disk
			// Combine contiguous extents

			if ((fcbArray[fd].file_entry->extents[i - 1].block_number + fcbArray[fd].file_entry->extents[i - 1].contiguous_blocks) == fcbArray[fd].file_entry->extents[i].block_number)
			{
				// these two extents are contiguous so merge them together
				fcbArray[fd].file_entry->extents[i - 1].contiguous_blocks += fcbArray[fd].file_entry->extents[i].contiguous_blocks;
				fcbArray[fd].file_entry->extents[i].contiguous_blocks = 0;
				fcbArray[fd].file_entry->extents[i].block_number = -1;
				printf("Merging extents\n");
			}

			// update the number of blocks that file has
			fcbArray[fd].num_blocks += numBlocksNeeded;
		}
	}

	int location = LBA_from_File_Block(blockDest, fcbArray[fd].file_entry);
	// printf("location on disk found from block desk at %d: %d\n", blockDest, location);

	if (LBAread(startBlock, 1, location) != 1)
	{
		printf("LBAread failed in b_write.\n");
		free(startBlock);
		return -1;
	}
	// printf("made it past first LBAread\n");
	// fill the remaining space in the startBlock
	int remainingSpaceInBlock = vcb->block_size - offset;
	memcpy(startBlock + offset, buffer, remainingSpaceInBlock);
	// update fileOffset

	remainingSpaceInBlock = (remainingSpaceInBlock > count) ? count : remainingSpaceInBlock;
	// update where we are in file
	b_seek(fd, remainingSpaceInBlock, SEEK_CUR);
	// update count
	count -= remainingSpaceInBlock;
	// commit the block to disk
	if (LBAwrite(startBlock, 1, location) != 1)
	{
		printf("LBAwrite failed in b_write\n");
		free(startBlock);
		return -1;
	}
	if (count <= 0)
	{
		// we are done
		// update the size of the file
		fcbArray[fd].file_entry->size += bytesWritten;
		int blocksForParent = (BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
		DE *parent = malloc(blocksForParent * vcb->block_size);
		if (parent == NULL)
		{
			puts("malloc failed in b_write for parent");
			return -1;
		}
		// load parent
		LBAread(parent, blocksForParent, fcbArray[fd].parent_location);
		// find what index file DE resides at and update the size
		for (int index = 0; index < BUFFER_SIZE; index++)
		{
			if (strcmp(parent[index].name, fcbArray[fd].file_entry->name) == 0)
			{
				// found file in parent so update file DE
				parent[index] = *fcbArray[fd].file_entry;
			}
		}
		LBAwrite(parent, blocksForParent, fcbArray[fd].parent_location);
		// b_seek(fd, bytesWritten, SEEK_CUR);
		free(startBlock);
		free(parent);

		return bytesWritten;
	}

	// update where we are in the buffer
	buffer += remainingSpaceInBlock;
	// we still have more content in the buffer to push

	while (count > 0)
	{

		// calculate location of next block in file on disk
		blockDest++;
		location = LBA_from_File_Block(blockDest, fcbArray[fd].file_entry);
		// read in the current block from disk
		if (LBAread(startBlock, 1, location) != 1)
		{
			printf("LBAread failed in b_write function\n");
			free(startBlock);
			return -1;
		}

		int remainingBytes = (count > vcb->block_size) ? vcb->block_size : count;
		// fill in the current block with contents in the buffer.
		memcpy(startBlock, buffer, remainingBytes);
		// write the block back to disc
		if (LBAwrite(startBlock, 1, location) != 1)
		{
			printf("LBAwrite failed in b_write function\n");
			free(startBlock);
			return -1;
		}
		// update the count
		count -= remainingBytes;
		// update the buffer position
		buffer += remainingBytes;
		b_seek(fd, remainingBytes, SEEK_CUR);
	}

	// update the size of the file depending on the amount of content written
	fcbArray[fd].file_entry->size += bytesWritten;

	int blocksForParent = (BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
	DE *parent = malloc(blocksForParent * vcb->block_size);
	if (parent == NULL)
	{
		puts("malloc failed in b_write for parent");
		return -1;
	}
	// load parent
	LBAread(parent, blocksForParent, fcbArray[fd].parent_location);
	// find what index file DE resides at and update the size
	for (int index = 0; index < BUFFER_SIZE; index++)
	{
		if (strcmp(parent[index].name, fcbArray[fd].file_entry->name) == 0)
		{
			// found file in parent so update file DE
			parent[index] = *fcbArray[fd].file_entry;
		}
	}

	// free resources
	free(parent);
	free(startBlock);

	return bytesWritten;
}

// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read(b_io_fd fd, char *buffer, int count)
{

	if (startup == 0)
		b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); // invalid file descriptor
	}
	// validate read permissions. If permission is 2, then file is WRITEONLY
	if (fcbArray[fd].permissions == 2)
	{
		printf("Do not have permission to read this file.\n");
		return -1;
	}

	// b_seek(fd, 0, SEEK_SET); //start at beginning of file TODO: remove later
	// get how much content is left in file from current place.
	// printf("size of the file being read is: %ld and the offset is: %d and the file name is: %s\n", fcbArray[fd].fileEntry->size, fcbArray[fd].fileOffset,
	// fcbArray[fd].fileEntry->name);

	int leftToRead = fcbArray[fd].file_entry->size - fcbArray[fd].file_offset;
	if (leftToRead <= 0)
	{
		printf("reached EOF\n");
		return 0;
	}
	// define index of user's buffer
	int userBufIndex = 0;

	// force count to be inside file dimensions
	if (count > leftToRead)
	{
		printf("executing partial read. \n");
		count = leftToRead;
	}
	// if nothing goes wrong, we know that current count is our return value
	int totalBytesRead = count;
	// get which block the offset resides in. This is our current block
	fcbArray[fd].current_block = fcbArray[fd].file_offset / B_CHUNK_SIZE;
	// calculate the current block number (on disk)
	int position = LBA_from_File_Block(fcbArray[fd].current_block, fcbArray[fd].file_entry);
	// get which byte of the current block we start at
	int startingByte = fcbArray[fd].file_offset % B_CHUNK_SIZE;

	// load the block from disk since we have the fileOffset, we can overwrite the fcb Buffer
	if (LBAread(fcbArray[fd].buf, 1, position) != 1)
	{
		printf("LBAread failed. Could not read from file.\n");
		return -1;
	}
	// we just read a block so now we are on the next block.
	fcbArray[fd].current_block++;
	// set the fcb buffer's index to the starting byte
	fcbArray[fd].index = startingByte;
	// read all bytes in fcb buffer starting at the index into the user's buffer
	// at the beginning of the user's buffer
	// or if count is smaller than the what buffer has left in it then just read count
	int amountToRead = (count < (B_CHUNK_SIZE - fcbArray[fd].index)) ? count : (B_CHUNK_SIZE - fcbArray[fd].index);
	// copy the bytes in to the user's buffer
	memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].index, amountToRead);
	userBufIndex += amountToRead;
	count -= amountToRead;
	if (count == 0)
	{
		// we finished count so we are done.
		// update the fileOffset
		b_seek(fd, totalBytesRead, SEEK_CUR);
		return totalBytesRead;
	}

	/* if the remaining count is bigger than one block, read in block directly
	/* to user's buffer until this is not true
	/* and then read remaining bytes using fcb.buf */

	if (count > B_CHUNK_SIZE)
	{
		int blocksToRead = count / B_CHUNK_SIZE;
		while (blocksToRead > 0)
		{
			// read the current block into the user's buffer directly
			if (LBAread(buffer + userBufIndex, 1, position) != 1)
			{
				printf("LBAread failed in b_read\n");
				return -1;
			}
			// update where we are in the user's buffer
			userBufIndex += B_CHUNK_SIZE;
			// we just read a block so go to the next one
			fcbArray[fd].current_block++;
			// calculate the block number of the next file block
			position = LBA_from_File_Block(fcbArray[fd].current_block, fcbArray[fd].file_entry);
			// we just read a block so decrement the blocksToRead
			blocksToRead--;
			// read in a whole block so update count
			count -= B_CHUNK_SIZE;
		}
		if (count == 0)
		{
			// read all bytes so we are done.
			// update the fileOffset
			b_seek(fd, totalBytesRead, SEEK_CUR);
			return totalBytesRead;
		}
	}
	// if we reach this line, then there still remain bytes left to read
	// and this remaining byte total is < the size of a block
	// so read the current block into the FCB buffer and then copy the exact amount of remaining
	// bytes left into user's buffer.

	// update currentBlock
	fcbArray[fd].current_block++;
	// get the next block to be read in
	position = LBA_from_File_Block(fcbArray[fd].current_block, fcbArray[fd].file_entry);
	// read in the block
	LBAread(fcbArray[fd].buf, 1, position);
	// just read a block so update currentBlk
	fcbArray[fd].current_block++;

	// copy bytes from file into buffer
	memcpy(buffer + userBufIndex, fcbArray[fd].buf, count);

	fcbArray[fd].bytes_read += totalBytesRead;
	// update the fileOffset
	b_seek(fd, totalBytesRead, SEEK_CUR);
	// return the amount of bytes read into user's buffer
	return totalBytesRead; // Change this
}

// Interface to Close the file
int b_close(b_io_fd fd)
{
}
//returns the address of a block on disk. It takes in a DirectoryEntry p of the file and the block you want from the file. 
//function taken directly from lecture slide of SFSU 415 operating systems class. Function written by Robert Bierman. 
int LBA_from_File_Block( int n, DE* p) { 
	int i = 0;
	//printf("name of file: %s and extent 0 count: %d and extent 0 blocknumber: %d\n", p->name, p->extents[0].count, p->extents[0].blockNumber);
	//loop through file's DE extent array
	//to arrive at the extent that contains the block n
	while (p->extents[i].contiguous_blocks <= n) {
		
		//use some of n to get to next extent
		n = n - p->extents[i].contiguous_blocks;
		//prepare to move to next extent in extent array
		i++; 
	} 
	//return the  disk number of first block of the extent that contains the block n
	//offset by the remainder of blocks.
	return p->extents[i].block_number + n; 
	}




	//creates a file. places file in param parent dir. Names file fileName. returns DE for file
	DE fs_mkfile(DE* parent, char fileName[20], int nameSize){
		//declare file's DE to be returned
		DE fileDE; 
		//get free index of parent/check if parent has space
		int index;
		for( index = 2; index < BUFFER_SIZE; ++index){
			if( parent[index].name[0] == '\0' ){
				//found free DE in parent dir for new file
				break;
			}
		}
		if(index == BUFFER_SIZE){
			printf("No room in parent directory. Did not create file.\n");
			fileDE.name[0] = '\0'; //indicate that file creation failed.
			return fileDE; 
		}
		//prepare to call allocateBlocks by loading bitmap
		
		//number of bytes in bitmap
		int bitmapSize =  ( ( vcb->num_blocks + 7) / 8 );
		loadBitmap();
		int DEnumBlocks =  (sizeof(DE) + vcb->block_size -1) / vcb->block_size;
		//int DElocation = allocateBlocks(DEnumBlocks, bitmap, bitmapSize);
		int location = allocateBlocks(FILE_INITIAL_NUM_BLOCKS, bitmap, bitmapSize);
		
		if(location < 0 ){ //allocateBlocks returns a negative on failure
			printf("failed to allocate blocks for new file. File not created\n");
			fileDE.name[0] = '\0'; //indicate that file creation failed.
			return fileDE; 
		}
		//initialize the file DE fields
		fileDE.extents[0].block_number = location;
		fileDE.extents[0].contiguous_blocks = FILE_INITIAL_NUM_BLOCKS;
		fileDE.extents[1].block_number = -1;
		fileDE.extents[1].contiguous_blocks = 0; 
		fileDE.extents[2].block_number = -1;
		fileDE.extents[2].contiguous_blocks = 0; 
		fileDE.extents[3].block_number = -1;
		fileDE.extents[3].contiguous_blocks = 0; 
		fileDE.extents[4].block_number = -1;
		fileDE.extents[4].contiguous_blocks = 0; 
		fileDE.extents[5].block_number = -1;
		fileDE.extents[5].contiguous_blocks = 0; 
		fileDE.extents[6].block_number = -1;
		fileDE.extents[6].contiguous_blocks = 0; 
		fileDE.extents[7].block_number = -1;
		fileDE.extents[7].contiguous_blocks = 0;  
		fileDE.isDirectory = 0;
		fileDE.loc = location;
		//file starts with no content
		fileDE.size = 0;
		//initialize the name
		
		strncpy(fileDE.name, fileName, nameSize);
		fileDE.name[nameSize] = '\0';
		//put new file in parent directory array
		parent[index] = fileDE; 
		//write the updated parent back to disk
		int numBlocks = (BUFFER_SIZE * sizeof(DE) + vcb->block_size -1) / vcb->block_size; 
		if(LBAwrite(parent, numBlocks, parent[0].loc) != numBlocks ){
			printf("error in LBAwrite. File may not have been created.\n");
			//indicate that the function failed by returning a fileDE with no name
			fileDE.name[0] = '\0';
		} 
		

		return fileDE;
	}

