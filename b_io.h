/**************************************************************
 * Class:  CSC-415-01 Summer 2023
 * Names:Anish Khadka, Joe Sand, Ameen Safi
 * Student IDs:921952002,920525382, 920689065
 * GitHub Name: asafi67
 * Group Name: File System Soldiers
 * Project: Basic File System
 *
 * File: b_io.h
 *
 * Description: This is a header file for b_io.c. This file defines
 * the interface for buffered I/O operations in the file system.
 * It includes essentail functions for managing files, such as opening
 * reading, writing, seeking, and closing.
 **************************************************************/
#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>
#include "rootDir.h"
#include "extent.h"

typedef int b_io_fd;

b_io_fd b_open(char *filename, int flags);
int b_read(b_io_fd fd, char *buffer, int count);
int b_write(b_io_fd fd, char *buffer, int count);
int b_seek(b_io_fd fd, off_t offset, int whence);
int b_close(b_io_fd fd);

// this function will create an actual file
// the parent parameter notes which directory to create a file in
// we also parameterize the name of file and its size
DE fs_mkfile(DE *parent, char file_name[], int name_size);

// Function to take in a file block and return a specific block number
int LBA_from_File_Block(int n, DE *p);

#endif
