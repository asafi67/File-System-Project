/**************************************************************
 * Class:  CSC-415-01 Summer 2023
 * Names:Anish Khadka, Joe Sand, Ameen Safi
 * Student IDs:921952002,920525382, 920689065
 * GitHub Name: asafi67
 * Group Name: File System Soldiers
 * Project: Basic File System
 *
 * File: b_io.c
 *
 * Description: Dedicated to the buffered I/O operations of the 
 * file system. It provides an interface for managing files within
 * the system, including file opening, initialization, and handling
 * File Control Blocks (FCB).
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

	// Check for error - all used FCB's
	char *pathCopy = malloc(strlen(filename) + 1);
	if (pathCopy == NULL)
	{ // check for successful allocation of pathCopy
		printf("Memory allocation failed in b_open\n");
		return -1;
	}

	strcpy(pathCopy, filename);

	// Parse the provided filename to get parent information
	PathReturn parentInfo = parsePath(filename);

	if (parentInfo.status_code == 0)
	{
		printf("File not found\n");
		return -1;
	}

	DE fileEntry;
	memset(&fileEntry, 0, sizeof(DE)); // Ensure fileEntry is intialized
	fileEntry.name[0] = '\0';

	if (parentInfo.status_code == 1) // Home/Desktop/newFile.txt
	{
		if ((flags & O_CREAT) == O_CREAT)
		{
			// The path does not exist yet, so create the file
			// Get the name of the new file to be created
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

			int i = 0;
			while (pathCopy[index] != '\0' && i < newNameSize)
			{
				newName[i] = pathCopy[index];
				i++;
				index++;
			}
			newName[i] = '\0';

			// Create the file with the specified name
			fileEntry = fs_mkfile(parentInfo.direc, newName, newNameSize);

			if (fileEntry.name[0] == '\0')
			{
				printf("Failed to make file.\n");
				return -1;
			}
		}
		else
		{
			printf("File not found.\n");
			return -1;
		}
	}
	else
	{
		if (parentInfo.direc[parentInfo.index_last].isDirectory == 1)
		{
			printf("Unable to open directory with that command\n");
			return -1;
		}

		// Get the file entry from the parent directory
		fileEntry = parentInfo.direc[parentInfo.index_last];
	}

	// Initialize the File Control Block (FCB) for the file
	fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
	fcbArray[returnFd].index = 0;
	fcbArray[returnFd].buflen = B_CHUNK_SIZE;
	fcbArray[returnFd].current_block = 0;
	fcbArray[returnFd].bytes_read = 0;
	fcbArray[returnFd].file_offset = 0;
	fcbArray[returnFd].parent_location = parentInfo.direc[0].loc;
	*fcbArray[returnFd].file_entry = fileEntry;

	int numberOfBlocks = 0;
	int i = 0;
	// Calculate the total number of blocks occupied by the file from its extents
	while (fcbArray[returnFd].file_entry->extents[i].contiguous_blocks > 0)
	{
		numberOfBlocks += fcbArray[returnFd].file_entry->extents[i].contiguous_blocks;
		++i;
	}
	printf("File location: %d and number of blocks: %d\n", fileEntry.loc, numberOfBlocks);
	fcbArray[returnFd].num_blocks = numberOfBlocks;
	fcbArray[returnFd].current_block = i;

	int permissions;
	if ((flags & O_RDONLY) == O_RDONLY)
	{
		permissions = 1; // Read-only permissions
	}
	else if ((flags & O_WRONLY) == O_WRONLY)
	{
		permissions = 2; // Write-only permissions
	}
	else if ((flags & O_RDWR) == O_RDWR)
	{
		permissions = 3; // Read and write permissions
	}

	// If O_TRUNC flag is set and file is not opened in read-only mode and O_CREAT flag is not set,
	// truncate the file by setting its size to 0.
	if (((flags & O_TRUNC) == O_TRUNC) && (permissions != 1) && ((flags & O_CREAT) != O_CREAT))
	{
		fileEntry.size = 0;
		parentInfo.direc[parentInfo.index_last] = fileEntry;
		printf("Truncating the file: %s\n", filename);

		// Write the modified parent directory back to disk
		int numBlocksForParent = (BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
		if (LBAwrite(parentInfo.direc, numBlocksForParent, parentInfo.direc[0].loc) != numBlocksForParent)
		{
			printf("LBAwrite failed in b_open. Did not truncate the file.\n");
			return -1;
		}
		fcbArray[returnFd].file_entry->size = 0;
	}

	// Print information about the file and its extents
	printf("Opening file %s extentLocation %d and location is: %d\n", fcbArray[returnFd].file_entry->name, fcbArray[returnFd].file_entry->extents[0].block_number, fcbArray[returnFd].file_entry->loc);
	fcbArray[returnFd].permissions = permissions;
	return returnFd; // Return the file descriptor for the opened file
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

	off_t newOffset = -1; // Variable to store the new offset position

	switch (whence)
	{
	case SEEK_SET:
		// SEEK_SET sets the file offset to 'offset' bytes from the beginning of the file.
		newOffset = offset;
		break;

	case SEEK_CUR:
		// SEEK_CUR sets the file offset to 'offset' bytes from the current position.
		newOffset = fcbArray[fd].file_offset + offset;
		break;

	case SEEK_END:
		// SEEK_END sets the file offset to 'offset' bytes from the end of the file.
		newOffset = fcbArray[fd].file_entry->size + offset;
		break;

	default:
		// Invalid 'whence' value, do nothing and keep the current offset.
		break;
	}

	if (newOffset >= 0)
	{
		// If the newOffset is valid (non-negative), update the file offset for the given file descriptor 'fd'.
		fcbArray[fd].file_offset = newOffset;
	}

	return newOffset; // Return the new offset position (or -1 in case of error).
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

	// Check if the file is read-only, in which case writing is not allowed
	if (fcbArray[fd].permissions == O_RDONLY)
	{
		printf("Writing to this file is not permitted.\n");
		return -1;
	}

	// bytes_wrote to be returned at the end of the function if nothing goes wrong
	int bytes_wrote = count;

	// Create a buffer for the first block
	char *start = malloc(B_CHUNK_SIZE);
	if (start == NULL)
	{
		printf("Memory allocation failed in b_write\n");
		return -1;
	}

	// Determine the block destination and the offset within the block
	int block_destination = fcbArray[fd].file_offset / vcb->block_size;
	int offset = fcbArray[fd].file_offset % vcb->block_size;

	// Check if we have enough space in the file to accommodate the specified count of bytes
	if ((fcbArray[fd].num_blocks * vcb->block_size - fcbArray[fd].file_offset) < count)
	{
		printf("File does not have enough space. Expanding the file...\n");
		// There is not enough space to fit count many bytes in the file.
		// Therefore, allocate more space for the file.

		// Calculate the number of blocks needed
		int required_bytes = count - (fcbArray[fd].num_blocks * vcb->block_size - fcbArray[fd].file_offset);
		// Convert bytes to blocks
		int required_blocks = (required_bytes + vcb->block_size - 1) / vcb->block_size;

		// Check if required_blocks is less than the default we would prescribe for the current extent
		extent nextExt;
		int i = 0;
		// Locate the next extent to be initialized in the file's extents array.
		while (fcbArray[fd].file_entry->extents[i].contiguous_blocks != 0)
		{
			i++;
		}

		// Formula to double the number of blocks of the previous extent for the current extent
		int extent_block_calc = 10 * (int)pow(2, i);
		required_blocks = (required_blocks > extent_block_calc) ? required_blocks : extent_block_calc;

		// Ask the bitmap for available space
		// First load the bitmap into memory if not already loaded
		if (loadBitmap() == -1)
		{
			printf("Failed to load the bitmap\n");
			free(start);
			return -1;
		}
		int bit_size = (vcb->num_blocks + 7) / 8;
		// Allocate space on disk (via the bitmap's allocateBlocks function) and set block_num to the returned value.
		int block_num = allocateBlocks(required_blocks, bitmap, bit_size);

		// If allocateBlocks returns a negative number, then update count to the remaining number of bytes in the file and proceed. (partial write MANPAGE)
		if (block_num < 0)
		{
			// Set count to the remaining number of bytes in the file. PARTIAL WRITE
			printf("Insufficient space on disk. Executing PARTIAL WRITE.\n");
			count = fcbArray[fd].num_blocks * vcb->block_size - fcbArray[fd].file_offset;
			bytes_wrote = count;
		}
		else
		{
			// If allocateBlocks found space for the new extent, then create an extent for the file

			nextExt.contiguous_blocks = required_blocks;
			nextExt.block_number = block_num;
			fcbArray[fd].file_entry->extents[i] = nextExt;

			// Merge extents if they occur contiguously on disk
			// Combine contiguous extents
			if ((fcbArray[fd].file_entry->extents[i - 1].block_number + fcbArray[fd].file_entry->extents[i - 1].contiguous_blocks) == fcbArray[fd].file_entry->extents[i].block_number)
			{
				// These two extents are contiguous, so merge them together
				fcbArray[fd].file_entry->extents[i - 1].contiguous_blocks += fcbArray[fd].file_entry->extents[i].contiguous_blocks;
				fcbArray[fd].file_entry->extents[i].contiguous_blocks = 0;
				fcbArray[fd].file_entry->extents[i].block_number = -1;
				printf("Merging extents\n");
			}

			// Update the number of blocks that the file has
			fcbArray[fd].num_blocks += required_blocks;
		}
	}

	// Calculate the location of the first block in the file on disk
	int location = LBA_from_File_Block(block_destination, fcbArray[fd].file_entry);

	if (LBAread(start, 1, location) != 1)
	{
		printf("Failed to read from disk in b_write\n");
		free(start);
		return -1;
	}

	// Fill the remaining space in the startBlock
	int space_left = vcb->block_size - offset;
	memcpy(start + offset, buffer, space_left);
	// Update fileOffset

	space_left = (space_left > count) ? count : space_left;
	// Update the current file position
	b_seek(fd, space_left, SEEK_CUR);
	// Update count
	count -= space_left;
	// Commit the block to disk
	if (LBAwrite(start, 1, location) != 1)
	{
		printf("Failed to write to disk in b_write\n");
		free(start);
		return -1;
	}

	if (count <= 0)
	{
		// We are done
		// Update the size of the file
		fcbArray[fd].file_entry->size += bytes_wrote;
		int parent_blocks = (BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
		DE *parent = malloc(parent_blocks * vcb->block_size);
		if (parent == NULL)
		{
			puts("Memory allocation failed for parent");
			return -1;
		}
		// Load parent
		LBAread(parent, parent_blocks, fcbArray[fd].parent_location);
		// Find the index where the file DE resides and update the size
		// Find the index where the file DE resides and update the size
		for (int index = 0; index < BUFFER_SIZE; index++)
		{
			if (strcmp(parent[index].name, fcbArray[fd].file_entry->name) == 0)
			{
				// Found the file in the parent directory, so update the file DE
				parent[index] = *fcbArray[fd].file_entry;
			}
		}

		// Write the updated parent directory back to disk
		LBAwrite(parent, parent_blocks, fcbArray[fd].parent_location);
		free(parent);
		free(start);

		return bytes_wrote;
	}

	// Update the buffer position, as there's still more content to write
	buffer += space_left;

	// We still have more content in the buffer to push to the disk
	while (count > 0)
	{
		// Calculate the location of the next block in the file on disk
		block_destination++;
		location = LBA_from_File_Block(block_destination, fcbArray[fd].file_entry);

		// Read in the current block from disk
		if (LBAread(start, 1, location) != 1)
		{
			printf("Failed to read from disk in b_write\n");
			free(start);
			return -1;
		}

		int bytes_left = (count > vcb->block_size) ? vcb->block_size : count;
		// Fill in the current block with contents from the buffer
		memcpy(start, buffer, bytes_left);
		// Write the block back to disk
		if (LBAwrite(start, 1, location) != 1)
		{
			printf("Failed to write to disk in b_write\n");
			free(start);
			return -1;
		}
		// Update the count
		count -= bytes_left;
		// Update the buffer position
		buffer += bytes_left;
		b_seek(fd, bytes_left, SEEK_CUR);
	}

	// Update the size of the file based on the amount of content written
	fcbArray[fd].file_entry->size += bytes_wrote;

	// Update the parent directory entry with the updated file size
	int parent_blocks = (BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
	DE *parent = malloc(parent_blocks * vcb->block_size);
	if (parent == NULL)
	{
		puts("Memory allocation failed for parent");
		return -1;
	}
	// Load the parent directory
	LBAread(parent, parent_blocks, fcbArray[fd].parent_location);
	// Find the index where the file DE resides and update the size
	for (int index = 0; index < BUFFER_SIZE; index++)
	{
		if (strcmp(parent[index].name, fcbArray[fd].file_entry->name) == 0)
		{
			// Found the file in the parent directory, so update the file DE
			parent[index] = *fcbArray[fd].file_entry;
		}
	}

	// Write the updated parent directory back to disk
	LBAwrite(parent, parent_blocks, fcbArray[fd].parent_location);
	free(parent);
	free(start);

	return bytes_wrote;
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
		printf("Permission required\n");
		return -1;
	}

	// get how much content is left in file from current place.
	int remaining_read = fcbArray[fd].file_entry->size - fcbArray[fd].file_offset;
	if (remaining_read <= 0)
	{
		printf("End of file has been reached\n");
		return 0;
	}
	// define index of user's buffer
	int index_user_buff = 0;

	// force count to be inside file dimensions
	if (count > remaining_read)
	{
		printf("Partial read execution \n");
		count = remaining_read;
	}
	// if nothing goes wrong, we know that current count is our return value
	int total_read = count;
	// get which block the offset resides in. This is our current block
	fcbArray[fd].current_block = fcbArray[fd].file_offset / B_CHUNK_SIZE;
	// calculate the current block number (on disk)
	int pos = LBA_from_File_Block(fcbArray[fd].current_block, fcbArray[fd].file_entry);
	// get which byte of the current block we start at
	int start_byte = fcbArray[fd].file_offset % B_CHUNK_SIZE;

	// load the block from disk since we have the fileOffset, we can overwrite the fcb Buffer
	if (LBAread(fcbArray[fd].buf, 1, pos) != 1)
	{
		printf("Failed LBAread\n");
		free(fcbArray[fd].buf); // Free buffer before return value is returned
		return -1;
	}
	// we just read a block so now we are on the next block.
	fcbArray[fd].current_block++;
	// set the fcb buffer's index to the starting byte
	fcbArray[fd].index = start_byte;
	// read all bytes in fcb buffer starting at the index into the user's buffer
	// at the beginning of the user's buffer
	// or if count is smaller than the what buffer has left in it then just read count
	int amountToRead = (count < (B_CHUNK_SIZE - fcbArray[fd].index)) ? count : (B_CHUNK_SIZE - fcbArray[fd].index);
	// copy the bytes in to the user's buffer
	memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].index, amountToRead);
	index_user_buff += amountToRead;
	count -= amountToRead;
	if (count == 0)
	{
		// we finished count so we are done.
		// update the fileOffset
		b_seek(fd, total_read, SEEK_CUR);
		return total_read;
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
			if (LBAread(buffer + index_user_buff, 1, pos) != 1)
			{
				printf("Failed LBAread n");
				return -1;
			}
			// update where we are in the user's buffer
			index_user_buff += B_CHUNK_SIZE;
			// we just read a block so go to the next one
			fcbArray[fd].current_block++;
			// calculate the block number of the next file block
			pos = LBA_from_File_Block(fcbArray[fd].current_block, fcbArray[fd].file_entry);
			// we just read a block so decrement the blocksToRead
			blocksToRead--;
			// read in a whole block so update count
			count -= B_CHUNK_SIZE;
		}
		if (count == 0)
		{
			// read all bytes so we are done.
			// update the fileOffset
			b_seek(fd, total_read, SEEK_CUR);
			return total_read;
		}
	}
	// if we reach this line, then there still remain bytes left to read
	// and this remaining byte total is < the size of a block
	// so read the current block into the FCB buffer and then copy the exact amount of remaining
	// bytes left into user's buffer.

	// update currentBlock
	fcbArray[fd].current_block++;
	// get the next block to be read in
	pos = LBA_from_File_Block(fcbArray[fd].current_block, fcbArray[fd].file_entry);
	// read in the block
	LBAread(fcbArray[fd].buf, 1, pos);
	// just read a block so update currentBlk
	fcbArray[fd].current_block++;

	// copy bytes from file into buffer
	memcpy(buffer + index_user_buff, fcbArray[fd].buf, count);

	fcbArray[fd].bytes_read += total_read;
	// update the fileOffset
	b_seek(fd, total_read, SEEK_CUR);
	// return the amount of bytes read into user's buffer
	return total_read; // Change this
}

// Interface to Close the file
int b_close(b_io_fd fd)
{
	if (fcbArray[fd].buf != NULL)
	{
		free(fcbArray[fd].buf);
		fcbArray[fd].buf = NULL;
	}
	return 1;
}
// returns the address of a block on disk. It takes in a DirectoryEntry p of the file and the block you want from the file.
// function taken directly from lecture slide of SFSU 415 operating systems class. Function written by Robert Bierman.
int LBA_from_File_Block(int blockNum, DE *fileEntry)
{
	int extentIndex = 0;

	while (fileEntry->extents[extentIndex].contiguous_blocks <= blockNum)
	{

		blockNum = blockNum - fileEntry->extents[extentIndex].contiguous_blocks;

		extentIndex++;
	}

	return fileEntry->extents[extentIndex].block_number + blockNum;
}

// creates a file. places file in param parent dir. Names file fileName. returns DE for file
DE fs_mkfile(DE *parent, char fileName[20], int nameSize)
{
	// declare file's DE to be returned
	DE file_directory_entry;
	// get free index of parent/check if parent has space
	int index;
	for (index = 2; index < BUFFER_SIZE; ++index)
	{
		if (parent[index].name[0] == '\0')
		{
			// found free DE in parent dir for new file
			break;
		}
	}
	if (index == BUFFER_SIZE)
	{
		printf("Parent directoy has no space, can't create file\n");
		file_directory_entry.name[0] = '\0'; // indicate that file creation failed.
		return file_directory_entry;
	}
	// prepare to call allocateBlocks by loading bitmap

	// number of bytes in bitmap
	int bitmap_size = ((vcb->num_blocks + 7) / 8);
	loadBitmap();
	int DE_block_count = (sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
	// int DElocation = allocateBlocks(DEnumBlocks, bitmap, bitmapSize);
	int location = allocateBlocks(FILE_INITIAL_NUM_BLOCKS, bitmap, bitmap_size);

	if (location < 0)
	{ // allocateBlocks returns a negative on failure
		printf("Can't allocate blocks for new file. File creation unsuccessful\n");
		file_directory_entry.name[0] = '\0'; // indicate that file creation failed.
		return file_directory_entry;
	}
	// initialize the file DE fields
	file_directory_entry.extents[0].block_number = location;
	file_directory_entry.extents[0].contiguous_blocks = FILE_INITIAL_NUM_BLOCKS;
	file_directory_entry.extents[1].block_number = -1;
	file_directory_entry.extents[1].contiguous_blocks = 0;
	file_directory_entry.extents[2].block_number = -1;
	file_directory_entry.extents[2].contiguous_blocks = 0;
	file_directory_entry.extents[3].block_number = -1;
	file_directory_entry.extents[3].contiguous_blocks = 0;
	file_directory_entry.extents[4].block_number = -1;
	file_directory_entry.extents[4].contiguous_blocks = 0;
	file_directory_entry.extents[5].block_number = -1;
	file_directory_entry.extents[5].contiguous_blocks = 0;
	file_directory_entry.extents[6].block_number = -1;
	file_directory_entry.extents[6].contiguous_blocks = 0;
	file_directory_entry.extents[7].block_number = -1;
	file_directory_entry.extents[7].contiguous_blocks = 0;
	file_directory_entry.isDirectory = 0;
	file_directory_entry.loc = location;
	// file starts with no content
	file_directory_entry.size = 0;
	// initialize the name

	strncpy(file_directory_entry.name, fileName, nameSize);
	file_directory_entry.name[nameSize] = '\0';
	// put new file in parent directory array
	parent[index] = file_directory_entry;
	// write the updated parent back to disk
	int numBlocks = (BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
	if (LBAwrite(parent, numBlocks, parent[0].loc) != numBlocks)
	{
		printf("LBAwrite failed\n");
		// indicate that the function failed by returning a file_DE with no name
		file_directory_entry.name[0] = '\0';
	}

	return file_directory_entry;
}
