#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

#include "mfs.h"
#include "volumeCB.h"
#include "rootDir.h"
#include "fsLow.h"
#include "bitmap.h"
#define BLOCK_SIZE 512

char* cwdString = "/";
DE* cwdPointer;

int fs_mkdir(const char *pathname, mode_t mode) {
    // Check if the directory already exists
    if (fs_isDir(pathname)) {
        // Directory already exists, return an error code
        return -1;
    }

    // Create a new directory entry in the parent directory
    char parentPath[256];
    char dirName[256];
    int indexLast;
    PathReturn parentPathReturn = parsePath(pathname);
    DE* parentDir = parentPathReturn.dir;

    if (parentDir == NULL) {
        // Parent directory not found, return an error code
        return -1;
    }

    // Get the name of the new directory
    sscanf(pathname + parentPathReturn.indexLast + 1, "%s", dirName);

    // Initialize the new directory
    int newDirLoc = initDir(2, parentDir);

    // Set the directory entry information
    strcpy(parentDir[parentDir[0].size / sizeof(DE)].name, dirName);
    parentDir[parentDir[0].size / sizeof(DE)].size = 0;
    parentDir[parentDir[0].size / sizeof(DE)].loc = newDirLoc;
    parentDir[parentDir[0].size / sizeof(DE)].isDirectory = 1;

    // Update the parent directory information
    parentDir[0].size += sizeof(DE);

    // Write the updated parent directory to disk
    LBAwrite(parentDir, parentDir[0].size, parentPathReturn.dir->loc);

    return 0; // Return success
}


int fs_rmdir(const char *pathname) {
    // Check if the directory exists and is a valid directory
    if (!fs_isDir(pathname)) {
        // Directory not found or not a directory, return an error code
        return -1;
    }

    PathReturn pathReturn = parsePath(pathname);
    DE* dir = pathReturn.dir;

    // Check if the directory is empty (only contains . and .. entries)
    if (dir[0].size > 2 * sizeof(DE)) {
        // Directory is not empty, return an error code
        return -1;
    }

    // Remove the directory entry from the parent directory
    DE* parentDir = pathReturn.dir;
    int entryIndex = pathReturn.indexLast;

    // Shift the directory entries to remove the entry
    for (int i = entryIndex; i < parentDir[0].size / sizeof(DE) - 1; i++) {
        parentDir[i] = parentDir[i + 1];
    }

    // Update the parent directory information
    parentDir[0].size -= sizeof(DE);

    // Write the updated parent directory to disk
    LBAwrite(parentDir, parentDir[0].size, parentDir->loc);

    // Mark the directory's blocks as unallocated
    freeBlocks(dir->loc);

    return 0; // Return success
}


fdDir * fs_opendir(const char *pathname) {
    // Check if the directory exists and is a valid directory
    if (!fs_isDir(pathname)) {
        // Directory not found or not a directory, return NULL
        return NULL;
    }

    // Allocate memory for the fdDir structure
    fdDir *dirp = (fdDir *)malloc(sizeof(fdDir));
    if (dirp == NULL) {
        // Memory allocation failed, return NULL
        return NULL;
    }

    // Parse the path and get the DE* for the directory
    PathReturn pathReturn = parsePath(pathname);
    DE* dir = pathReturn.dir;

    // Initialize the fdDir structure
    dirp->d_reclen = sizeof(struct fs_diriteminfo);
    dirp->dirEntryPosition = 0;
    dirp->dirStartLocation = dir->loc;
    dirp->directory = dir;
    dirp->di = (struct fs_diriteminfo *)malloc(dirp->d_reclen);
    if (dirp->di == NULL) {
        // Memory allocation failed, free dirp and return NULL
        free(dirp);
        return NULL;
    }

    return dirp; // Return the fdDir structure
}
struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
    // Check if the directory stream is NULL or at the end of the directory
    if (dirp == NULL || dirp->dirEntryPosition >= dirp->directory[0].size / sizeof(DE)) {
        return NULL; // Return NULL to indicate the end of the directory
    }

    // Get the current directory entry
    DE* currentEntry = &(dirp->directory[dirp->dirEntryPosition]);

    // Fill the fs_diriteminfo structure with the current entry information
    dirp->di->d_reclen = sizeof(struct fs_diriteminfo);
    dirp->di->fileType = (currentEntry->isDirectory) ? FT_DIRECTORY : FT_REGFILE;
    strcpy(dirp->di->d_name, currentEntry->name);

    // Move to the next directory entry
    dirp->dirEntryPosition++;

    return dirp->di; // Return the fs_diriteminfo structure
}
int fs_closedir(fdDir *dirp) {
    // Check if the directory stream is NULL
    if (dirp == NULL) {
        return -1; // Return an error code
    }

    // Free the dynamically allocated memory
    free(dirp->di);
    free(dirp);

    return 0; // Return success
}


char * fs_getcwd(char *pathname, size_t size) {
    if (pathname == NULL || size == 0) {
        return NULL; // Return NULL for invalid arguments
    }

    // Get the current directory entry
    DE* currentDir = &(cwdPointer);

    // Copy the current directory path to the provided buffer
    strncpy(pathname, currentDir->name, size);

    return pathname; // Return the buffer containing the current working directory path
}


int fs_setcwd(char *pathname) {
    // Parse the path and get the DE* for the directory
    PathReturn pathReturn = parsePath(pathname);
    DE* dir = pathReturn.dir;

    // Check if the directory exists and is a valid directory
    if (dir == NULL || !dir->isDirectory) {
        return -1; // Directory not found or not a valid directory, return an error code
    }

    // Update the current working directory location
  
  
  cwdPointer = dir->loc;

    return 0; // Return success
}


int fs_isFile(char *filename) {
    // Parse the path and get the DE* for the file
    PathReturn pathReturn = parsePath(filename);
    DE* file = pathReturn.dir;

    // Check if the file exists and is a valid regular file
    if (file == NULL || file->isDirectory) {
        return 0; // File not found or not a valid regular file, return 0 (false)
    }

    return 1; // Return 1 (true) to indicate that the given path is a regular file
}


int fs_isDir(char *pathname) {
    // Parse the path and get the DE* for the directory
    PathReturn pathReturn = parsePath(pathname);
    DE* dir = pathReturn.dir;

    // Check if the directory exists and is a valid directory
    if (dir == NULL || !dir->isDirectory) {
        return 0; // Directory not found or not a valid directory, return 0 (false)
    }

    return 1; // Return 1 (true) to indicate that the given path is a directory
}
int fs_delete(char* filename) {
    // Parse the path and get the DE* for the file
    PathReturn pathReturn = parsePath(filename);
    DE* file = pathReturn.dir;

    // Check if the file exists and is a valid regular file
    if (file == NULL || file->isDirectory) {
        return -1; // File not found or not a valid regular file, return an error code
    }

    // Remove the file entry from the parent directory
    DE* parentDir = pathReturn.dir;
    int entryIndex = pathReturn.indexLast;

    // Shift the directory entries to remove the entry
    for (int i = entryIndex; i < parentDir[0].size / sizeof(DE) - 1; i++) {
        parentDir[i] = parentDir[i + 1];
    }

    // Update the parent directory information
    parentDir[0].size -= sizeof(DE);

    // Write the updated parent directory to disk
    LBAwrite(parentDir, parentDir[0].size, parentDir->loc);

    // Mark the file's blocks as unallocated
    freeBlocks(file->loc);

    return 0; // Return success
}


int fs_stat(const char *path, struct fs_stat *buf) {
    // Parse the path and get the DE* for the file/directory
    PathReturn pathReturn = parsePath(path);
    DE* entry = pathReturn.dir;

    // Check if the entry exists
    if (entry == NULL) {
        return -1; // Entry not found, return an error code
    }

    // Fill the fs_stat structure with the entry information
    buf->st_size = entry->size;
    buf->st_blksize = BLOCK_SIZE;
    buf->st_blocks = (entry->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    buf->st_accesstime = entry->lastAccessedDate;
    buf->st_modtime = entry->modifiedAt;
    buf->st_createtime = entry->createdAt;

    // Add additional attributes here if needed for your file system

    return 0; // Return success
}
