/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
* Student ID: N/A
* Project: Basic File System
*
* File: mfs.h
*
* Description: 
*	
*
**************************************************************/
#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"
#include "rootDir.h"

#include <dirent.h>
#define FT_REGFILE	   DT_REG
#define FT_DIRECTORY   DT_DIR
#define FT_LINK	DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

 extern DE* cwdPointer; 
 extern char* cwdString;

struct fs_diriteminfo{
    unsigned short d_reclen;    // The length of this record.
    unsigned char fileType;    // The type of the file.
    char d_name[256]; 		// The file name (maximum length is 255 characters).
	};

typedef struct fdDir{
	unsigned short  d_reclen;		// Length of this record.
	unsigned short	dirEntryPosition; //directory entry position
	uint64_t	directoryStartLocation;		//starting LBA
	DE* directory; //The directory array
	} fdDir;

// This struct packages the status code and directory entry (DE) pointer as a return value
typedef struct PathReturn{
	DE* direc; // Pointer to a directory entry (e.g., for A/B/C, it would point to B).
	int status_code; // Status of the path passed to parsePath
	int index_last;  // Index of the last token in the path. 
}PathReturn;

// Function that takes a file path and returns whether the path is valid,
// the status of the last token, and a pointer to the last directory.
struct PathReturn parsePath(char* path);

//functions to manage directories
int fs_mkdir(const char *pathname, mode_t mode);  //make a directory
int fs_rmdir(const char *pathname);               //remove a directory
fdDir * fs_opendir(const char *pathname);         //open a directory
struct fs_diriteminfo *fs_readdir(fdDir *dirp);   //Read a directory
int fs_closedir(fdDir *dirp);                    //close a directory

// Miscellaneous directory functions
char * fs_getcwd(char *pathname, size_t size); //get the current working directory
int fs_setcwd(char *pathname);   //change the current working directory
int fs_isFile(char * filename);	 //return 1 if file, 0 otherwise
int fs_isDir(char * pathname);   //return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//remove a file


// Strucutre that is filled in from a call to fs_stat
struct fs_stat{
	off_t     st_size;    //total size, in bytes
	blksize_t st_blksize; 	//Block size for file system I/O
	blkcnt_t  st_blocks;  		//number of 512B blocks allocated
	time_t    st_accesstime;   	//time of last access
	time_t    st_modtime;   	//time of last modification
	time_t    st_createtime;   //time to last status change
	};

int fs_stat(const char *path, struct fs_stat *buf);

#endif

