#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

#include "mfs.h"
#include "volumeCB.h"
#include "rootDir.h"
#include "fsLow.h"
#include "bitmap.h"
#define BLOCK_SIZE 512

// Allocates memory for the current working directory "cwd" string
char *cwdString = "/";
DE *cwdPointer;

// parsePath function to parse a given file path, returning status codes:
// 0 - Invalid path
// 1 - Valid path, last part does not exist
// 2 - Valid path, last part does exist
// Also returns the parent directoryEntry;
PathReturn parsePath(char *path)
{

    PathReturn pathRet;     // structure to hold the result of path parsing
    pathRet.index_last = 0; // set last index to 0 because it starts empty

    // allocate memory for the root
    DE *root = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1);

    // check if allocation worked
    if (root == NULL)
    {
        // error message and update status code
        printf("malloc failed in parsePath\n");
        pathRet.status_code = -1;
        return pathRet;
    }

    // Determine if the given path is absolute or relative and start accordingly
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;
    // if path starts with '/' is an absolute path
    if (path[0] == '/')
    {
        LBAread(root, numBlocks, vcb->root_index); // Start at root for absolute path
    }
    // else path is a relative path
    else
    {
        LBAread(root, numBlocks, cwdPointer[0].loc); // Start at cwd for relative path
    }

    pathRet.direc = root;                   // set the starting directory
    char *currentToken = strtok(path, "/"); // tokenize the path
    int found;                              // flag for if a child directory was found

    // loop through the path, directory by directory
    while (currentToken != NULL)
    {

        found = 0; // reset found flag

        // loop through current directory for child
        for (int i = 0; i < BUFFER_SIZE; ++i)
        {
            if (strcmp(pathRet.direc[i].name, currentToken) == 0)
            {

                // Match found; set found as valid (1)
                found = 1;
                // Allocate memory for next directory
                DE *nextDir = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1);

                // validate memory allocation for next directory
                if (nextDir == NULL)
                {
                    printf("malloc failed in parsepath function\n");
                    pathRet.status_code = -1;
                    return pathRet;
                }

                // Check if the current token is a file
                if (pathRet.direc[i].isDirectory == 0)
                {
                    currentToken = strtok(NULL, "/");
                    // check if its the last token
                    if (currentToken == NULL)
                    {
                        // path is valid and file exists
                        pathRet.status_code = 2;
                        pathRet.index_last = i;
                        return pathRet;
                    }
                    // if not, path is invalid as it includes a file in the middle
                    pathRet.status_code = 0;
                    return pathRet;
                }
                // Current token is a directory
                LBAread(nextDir, numBlocks, pathRet.direc[i].loc);
                pathRet.direc = nextDir;
                pathRet.index_last = i;
                break; // Exit loop
            }
        }
        // check if no match was found in the current directory
        if (found != 1)
        {
            currentToken = strtok(NULL, "/");
            // If it's the last token, path is valid and file exists
            if (currentToken == NULL)
            {
                pathRet.status_code = 1;
                return pathRet;
            }
            // If not the last token, path is invalid
            pathRet.status_code = 0;
            return pathRet;
        }
        // continue to next part in the path
        currentToken = strtok(NULL, "/");
    }
    // If all tokens are valid and exist, return the parent of the last entry
    pathRet.status_code = 2;
    if (LBAread(pathRet.direc, numBlocks, pathRet.direc[1].loc) != numBlocks)
    {
        printf("LBAread failed in parsePath \n");
        pathRet.status_code = -10;
    }
    return pathRet;
}

// fs_getcwd gets the current directory where the user is located
char *fs_getcwd(char *pathname, size_t size)
{
    return cwdString;
}

// function calculates the canonical form of an absolute path
char *getCanonicalPath(char *path)
{
    char newPath[1000] = {};   // Holds the resulting canonical path
    char *stack[strlen(path)]; // Used for handiling the ionput path
    char *saveptr;             // Required for strtok_r
    int top = -1;              // Maintains the current position within the stack

    char *token = strtok_r(path, "/", &saveptr); // tokenize the path

    // loop through the path
    while (token != NULL)
    {
        // check if its ".."
        if (strcmp(token, "..") == 0)
        {
            // Go one level up if the stack isn't empty by popping the stack
            if (top > -1)
            {
                top--;
            }
        }
        // check if its "."
        else if (strcmp(token, ".") == 0)
        {
        }
        else
        {
            // The token is a valid element
            stack[++top] = token; // push onto the stack
        }
        // Get the next token
        token = strtok_r(NULL, "/", &saveptr);
    }

    newPath[0] = '/'; // starting with a leading slash
    // Iterate through the stack and build the canonical path
    for (int i = 0; i <= top; ++i)
    {
        strcat(newPath, stack[i]);

        if (i != top)
        {
            // keep going, append a slash
            strcat(newPath, "/");
        }
        else
        {
            // end, append a null terminator
            strcat(newPath, "\0");
        }
    }
    // Allocate memory for the result string and copy the newly constructed path
    char *result = (char *)malloc(strlen(newPath) * sizeof(char) + 1);
    strcpy(result, newPath);
    // return the canonical path
    return result;
}
// Function to change the current working directory
int fs_setcwd(char *pathname)
{

    // Allocate and create a copy of pathname for parsePath to modify
    char *copyOfPath = malloc(strlen(pathname) + 1);
    strcpy(copyOfPath, pathname);

    // Parse the provided pathname
    PathReturn res;
    res = parsePath(pathname);

    // check for failure in parsing
    if (res.status_code == -1)
    {
        printf("failed to parse the pathname given to fs_setcwd\n");
        if (res.direc != NULL)
            free(res.direc);
        res.direc = NULL;
        free(copyOfPath);
        return -1;
    }
    // Check for an invalid status code
    if (res.status_code != 2)
    {
        printf("Invalid path given to fs_setcwd\n");
        free(copyOfPath);
        if (res.direc != NULL)
            free(res.direc);
        res.direc = NULL;
        return -1;
    }
    // check if the path points to a file instead of a directory
    if (res.direc[res.index_last].isDirectory != 1)
    {
        printf("Invalid path given to fs_setcwd\n");
        free(copyOfPath);
        if (res.direc != NULL)
            free(res.direc);
        res.direc = NULL;
        return -1;
    }

    // If we reach here the provided path is valid
    // Determine if the pathname is relative or absolute
    if (copyOfPath[0] != '/')
    {
        // Allocate a new string large enough to hold the concatenated path
        char newCwdString[strlen(cwdString) + 2 + strlen(copyOfPath)];
        strcpy(newCwdString, cwdString);            // Copy the cwd into the new string
        strcat(newCwdString, "/");                  // Append a slash separator
        strcat(newCwdString, copyOfPath);           // Append the relative path
        cwdString = getCanonicalPath(newCwdString); // Obtain the canonical path and update cwd
    }
    else
    {
        printf("path is %s\n", copyOfPath);
        // Handling absolute pathname
        cwdString = getCanonicalPath(copyOfPath);
    }
    // Determine the required block size for cwdPointer
    int cwdBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;
    if (LBAread(cwdPointer, cwdBlocks, res.direc[res.index_last].loc) != cwdBlocks)
    {
        printf("LBAread operation failed to delete function\n");
    }
    free(copyOfPath);
    if (res.direc != NULL)
        free(res.direc);
    res.direc = NULL;
    return 0;
}

// Function to create a new directory; returns 1 if successful, -1 if failed
int fs_mkdir(const char *pathname, mode_t mode)
{
    // Identify the starting index of the last element in the path
    char copy[strlen(pathname)];
    strcpy(copy, pathname);
    int index = 0;
    int parent_index;
    // Iterate through the path string
    for (int i = strlen(copy); i >= 0; i--)
    {
        if (copy[i] == '/')
        {
            index = ++i; // Set index to the position after the last slash
            break;
        }
    }

    PathReturn parent;
    parent = parsePath((char *)pathname); // parsePath to validate the path
    if (parent.status_code != 1)
    {
        printf("Provided path is incorrect %s\n", copy);
        if (parent.status_code == 2)
        {
            printf("This directory: %s already exists: \n", pathname + index);
        }
        if (parent.direc != NULL)
            free(parent.direc);
        parent.direc = NULL;
        return -1;
    }
    // Extract the name of the new directory from the given path
    char new_directory_name[strlen(copy) - index + 1];
    strncpy(new_directory_name, copy + index, strlen(copy) - index + 1);

    // check if the name is the same name as the parent
    if (strcmp(new_directory_name, parent.direc[0].name) == 0)
    {
        printf("Operation denied-- directory %s was not created\n", new_directory_name);
        return -1;
    }

    // verify if there is available space in the parent directory
    for (int i = 2; i < BUFFER_SIZE; i++)
    {
        if (strlen(parent.direc[i].name) == 0)
        {
            // Reserve blocks on disk for the new directory and update bitmap
            int new_loc = initDir(parent.direc, vcb->block_size);
            if (new_loc == -1)
            {
                printf("An error occurred during the initialization of the directory\n");
                if (parent.direc != NULL)
                    free(parent.direc);
                parent.direc = NULL;
                return -1;
            }
            // initalize new directory
            DE newD;
            // set properties for the new directory
            newD.isDirectory = 1;
            newD.created_at = time(NULL);
            newD.last_accessed = time(NULL);
            newD.modified_at = time(NULL);
            newD.loc = new_loc;

            // assign the name for the new directory
            strncpy(newD.name, new_directory_name, sizeof(newD.name));
            parent.direc[i] = newD;

            // Commit the updated parent directory information to the disk
            int parent_blocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;
            LBAwrite(parent.direc, parent_blocks, parent.direc->loc);
            if (parent.direc != NULL)
                free(parent.direc);
            parent.direc = NULL;
            // Return a success '1'
            return 1;
        }
    }
    // If we are here then there is no available space in the parent directory
    printf("insufficient space in the parent directory\n");
    if (parent.direc != NULL)
        free(parent.direc);
    
    parent.direc = NULL;
    // return a failure '-1'
    return -1;
}

// function checks if the given path corresponds to a directory
int fs_isDir(char *pathname)
{

    // parsePath function to analyze the given pathname
    PathReturn result = parsePath(pathname);

    // Assess the status code returned by parsePath
    if (result.status_code == -1)
    {
        printf("Failed to parse the path\n");
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return -1; // return -1 to indicate invalid path
    }
    else if (result.status_code < 2)
    {
        printf("The provided path is not valid\n");
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return -1; // return -1 to indicate invalid path
    }
    // If the path is valid, extract the information about the actual element
    // Determine whether the element is a directory or not
    int res = result.direc[result.index_last].isDirectory;
    if (result.direc != NULL)
        free(result.direc);
    result.direc = NULL;

    // return the result (1 for directory, 0 for file)
    return res;
}

// Function checks if the given path corresponds to a file.
int fs_isFile(char *filename)
{
    int result = fs_isDir(filename); // fs_isDir to determine if path corresponds to a directory
    // check if result is negative
    if (result < 0)
    {
        return result;
    }
    // If result is 1, return 0 (not a file), else return 1 (is a file)
    return (result == 1) ? 0 : 1;
}

fdDir *fs_opendir(const char *pathname)
{
    // parsePath to analyze the given path
    PathReturn result = parsePath((char *)pathname);
    // check the status code is negative 1
    if (result.status_code == -1)
    {
        printf("Failed to parse the path\n");
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return NULL;
    }
    // check the status code is positive but not 1 or 0
    else if (result.status_code < 2)
    {
        printf("The provided path is not valid\n");
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return NULL;
    }
    // Allocate memory for file stat structure
    struct fs_stat *stat = malloc(sizeof(struct fs_stat));
    if (stat == NULL)
    {
        printf("Memory allocation failed in fs_opendir. Directory not opened.\n");
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return NULL;
    }
    fs_stat(pathname, stat);            // Get the stat of the specified path
    fdDir *dir = malloc(sizeof(fdDir)); // Allocate memory for directory entry
    if (dir == NULL)
    {
        printf("error in malloc occurred in fs_opendir function\n");
    }

    dir->d_reclen = stat->st_size;                                           // set the record length
    dir->directoryStartLocation = result.direc[result.index_last].loc;       // set the start location
    dir->dirEntryPosition = 0;                                               // initalize the directory entry position
    dir->directory = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1); // allocate memory

    // compute the number of blocks
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;

    if (LBAread(dir->directory, numBlocks, dir->directoryStartLocation) != numBlocks)
    {
        printf("LBAread operation failed in opendir function\n");
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return NULL;
    }
    if (result.direc != NULL)
        free(result.direc);
    result.direc = NULL;

    // return the directory entry
    return dir;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    // check if the directory pointer is NULL
    if (dirp == NULL)
    {
        printf("No valid directory item provided.\n");
        return NULL;
    }
    // check if the directory entry position has reached beyond the buffer size
    if (dirp->dirEntryPosition > BUFFER_SIZE)
    {
        return NULL; // return NULL, end of the directory reached
    }

    // Allocate memory for the directory item info structure to be returned
    struct fs_diriteminfo *result = malloc(sizeof(struct fs_diriteminfo));
    result->d_reclen = sizeof(struct fs_diriteminfo); // Assign the size of
    result->fileType = (dirp->directory[dirp->dirEntryPosition].isDirectory == 1)
                           ? FT_DIRECTORY : FT_REGFILE; // Determine the file type

    // Copy the name of the current directory item
    strncpy(result->d_name, dirp->directory[dirp->dirEntryPosition].name, sizeof(result->d_name));
    // Increment to the next directory entry position
    int i = dirp->dirEntryPosition;
    char *currName = dirp->directory[++dirp->dirEntryPosition].name;

    // Iterate until a valid directory entry is found or the end of buffer is reached
    while (strlen(currName) == 0 && (i < BUFFER_SIZE))
    {
        dirp->dirEntryPosition++;
        i++;
        currName = dirp->directory[dirp->dirEntryPosition].name;
    }
    // return the directory item info structure
    return result;
}

// function to close a directory
int fs_closedir(fdDir *dirp)
{
    // release the memory allocated for the directory entries
    free(dirp->directory);
    dirp->directory = NULL;
    // release the memory allocated for the directory stream pointer itself
    free(dirp);
    // return success indicator (1)
    return 1;
}

// function to remove a file
int fs_delete(char *filename)
{

    // reload the current working directory pointer
    int blocksForcwd = (BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1) / vcb->block_size;

    // check if LBAread works
    if (LBAread(cwdPointer, blocksForcwd, cwdPointer[0].loc) != blocksForcwd)
    {
        puts("Failed to read LBA in fs_delete");
        return 0;
    }
    // Iterate through the current working directory to find the file
    for (int i = 2; i < BUFFER_SIZE; ++i)
    {
        if (strcmp(cwdPointer[i].name, filename) == 0)
        {
            for (int currentExt = 0; currentExt < 8; currentExt++)
            {
                if (cwdPointer[i].extents[currentExt].contiguous_blocks != 0)
                    releaseBlocks(cwdPointer[i].extents[currentExt].contiguous_blocks,
                                  cwdPointer[i].extents[currentExt].block_number);
            }

            // Free the blocks for the Directory Entry (DE) for the file as well
            int fileDEBlocks = (sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
            releaseBlocks(fileDEBlocks, cwdPointer[i].loc);

            // Remove the file from the parent directory by clearing its name
            cwdPointer[i].name[0] = '\0';

            // Write the updated parent directory back to disk
            if (LBAwrite(cwdPointer, (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size, cwdPointer[0].loc) != (sizeof(DE) * BUFFER_SIZE +
                                                                                                                                vcb->block_size - 1) /
                                                                                                                                   vcb->block_size)
            {
                printf("Failed to write to LBA in fs_delete.\n");
                return -1; // return failure (-1)
            }

            return 1; // return success as file deletion was successful (1)
        }
    }
    // File was not found in parent directory
    printf("File: %s was not found in cwd: %s File was not deleted.\n", filename, cwdString);
    return 0; // return failure (0)
}

// Function to remove a directory or afile
int fs_rmdir(const char *pathname)
{

    // Parse the given path to obtain info
    PathReturn result = parsePath((char *)pathname);

    // check if the path is invalid
    if (result.status_code != 2)
    {
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return -1; // return failure
    }

    // Allocate memory for the directory entry to be removed
    DE *toBeRemoved = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1);

    // Check for memory allocation failure
    if (toBeRemoved == NULL)
    {
        printf("Memory allocation failed in fs_rmdir\n");
        if (result.direc != NULL)
        { // Free directory pointer if not NULL
            free(result.direc);
            result.direc = NULL;
        }
        return -1; // return failure
    }

    // calculate the number of blocks needed
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;

    // read the logical block address of the directory or file to be removed
    LBAread(toBeRemoved, numBlocks, result.direc[result.index_last].loc);

    // check if the last elemnt in the path is a file
    if (result.direc[result.index_last].isDirectory == 0)
    {
        // delete it using fs_delete
        fs_delete(result.direc[result.index_last].name);
    }

    // Path is legal so now we check if the folder is not empty
    for (int i = 2; i < BUFFER_SIZE; ++i)
    {
        if (strlen(toBeRemoved[i].name) != 0)
        {
            printf("Error removing directory, directory may not be empty\n");
            if (result.direc != NULL)
            {
                free(result.direc);
                result.direc = NULL;
            }
            return -1; // return failure
        }
    }
    // the directory is confirmed to be empty. Update the free space structure
    // to reflect that the blocks of the removed directory are now free
    releaseBlocks(numBlocks, result.direc[result.index_last].loc);

    // Delete the directory's directory entry (DE) in the parent
    result.direc[result.index_last].name[0] = '\0';

    // Write the updated parent directory information back to disk
    if (LBAwrite(result.direc, numBlocks, result.direc->loc) != numBlocks)
    {
        printf("failed to use LBAwrite in fs_rmdir \n");
    }
    if (result.direc != NULL)
        free(result.direc);
    result.direc = NULL;

    return 1; // return success, directory has been removed
}

// Function populates buffer with information about the specified path
int fs_stat(const char *path, struct fs_stat *buffer)
{
    // parsePath to return a status code
    PathReturn returnValue = parsePath((char *)path);

    // check if the status code is not 2
    if (returnValue.status_code != 2)
    {
        // path is illegal print error message
        printf("path: %s is not valid. The buffer has NOT been populated\n", path);
        if (returnValue.direc != NULL)
            free(returnValue.direc);
        returnValue.direc = NULL;
        return 0; // return failure
    }
    // Retrieve the last element from the parsed path
    DE lastElement = returnValue.direc[returnValue.index_last];
    // Calculate the size of the last element, whether it's a directory or file.
    buffer->st_size = (returnValue.direc[returnValue.index_last].isDirectory == 1)
                          ? BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1
                          : lastElement.size;

    // populate other fields in the buffer with the information about the last element.
    buffer->st_blksize = vcb->block_size;
    buffer->st_blocks = (lastElement.size + vcb->block_size - 1) / vcb->block_size;
    buffer->st_accesstime = lastElement.last_accessed || (time_t)0;
    buffer->st_modtime = lastElement.modified_at || (time_t)0;
    buffer->st_createtime = lastElement.created_at || (time_t)0;

    // check if the directory pointer is not NULL
    if (returnValue.direc != NULL)
        // free the directory pointer
        free(returnValue.direc);
    returnValue.direc = NULL;

    return 1; // return success
}

// Function move
int fs_mv(char *sPath, char *dPath)
{
    // parsePath the source path
    PathReturn source = parsePath(sPath);

    // check if the status code is not 2
    if (source.status_code != 2)
    {
        printf("invalid path given\n");
        if (source.direc != NULL)
            free(source.direc);
        source.direc = NULL;
        return -1; // return failure
    }

    // Compute the number of blocks required.
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;

    // parsePath the destination path
    PathReturn destination = parsePath(dPath);

    // check if the destination path is valid but last element does not exist
    if (destination.status_code == 1)
    {

        // rename source to destination
        int nameLength = sizeof(source.direc[source.index_last].name);
        strncpy(source.direc[source.index_last].name, dPath, nameLength);

        // write the change to disk
        if (LBAwrite(source.direc, numBlocks, source.direc[0].loc) != numBlocks)
        {
            // check if LBAwrite failed
            printf("LBAwrite failed in fs_mv\n");
            if (source.direc != NULL)
                free(source.direc);
            source.direc = NULL;
            return -1; // return failure
        }

        // Free allocated resources
        if (destination.direc != NULL)
            free(destination.direc);
        destination.direc = NULL;
        if (source.direc != NULL)
            free(source.direc);
        source.direc = NULL;
        return 1; // return success
    }

    // Check if the destination does exist, proceed based on its type (file or directory)
    if (destination.status_code == 2)
    {
        // Check if dest is not a directory and validate that source is not a directory
        if (destination.direc[destination.index_last].isDirectory != 1)
        {

            // Check if source is not a directory
            if (source.direc[source.index_last].isDirectory == 1)
            {
                printf("cannot move a directory into a file.\n");
                if (source.direc != NULL)
                    free(source.direc);
                source.direc = NULL;
                return -1; // return failure
            }

            // Replace the destination entry with the source entry, effectively moving it.
            destination.direc[destination.index_last] = source.direc[source.index_last];
            source.direc[source.index_last].name[0] = '\0';

            // Write changes back to disk with LBAwrite
            if (LBAwrite(destination.direc, numBlocks, destination.direc[0].loc) != numBlocks)
            {
                puts("LBAwrite failed in fs_mv");
                return -1;
            }
            if (LBAwrite(source.direc, numBlocks, source.direc[0].loc) != numBlocks)
            {
                puts("LBAwrite failed in fs_mv");
                return -1;
            }
            return 1;
        }
        // Get the canonical path to the source
        char *pathToSrc = getCanonicalPath(sPath);
        // load the destination directory from disk
        DE *destinationDir = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1);
        // check if the load worked
        if (destinationDir == NULL)
        {
            printf("malloc failed in fs_mv\n");
            if (source.direc != NULL)
                free(source.direc);
            source.direc = NULL;
            return -1;
        }
        // Read the destination directory from disk
        if (LBAread(destinationDir, numBlocks, destination.direc[destination.index_last].loc) != numBlocks)
        {
            printf("LBAread failed in fs_mv\n");
            if (source.direc != NULL)
                free(source.direc);
            source.direc = NULL;
            return -1;
        }
        // loop for a free space in the destination directory and move source into it
        for (int i = 2; i < BUFFER_SIZE; ++i)
        {
            if (strlen(destinationDir[i].name) == 0)
            {
                printf("the name of the dir to move is: %s and it its isDir is: %d\n",
                       source.direc[source.index_last].name, source.direc[source.index_last].isDirectory);
                destinationDir[i] = source.direc[source.index_last];
                // Write changes to disk
                if (LBAwrite(destinationDir, numBlocks, destinationDir[0].loc) != numBlocks)
                {
                    printf("error in LBAwrite in fs_mv function\n");
                    if (source.direc != NULL)
                    {
                        free(source.direc);
                        source.direc = NULL;
                    }
                    return -1;
                }
                source.direc[source.index_last].name[0] = '\0';
                // Write changes to disk
                if (LBAwrite(source.direc, numBlocks, source.direc[0].loc) != numBlocks)
                {
                    printf("error in LBAwrite in fs_mv function\n");
                    if (source.direc != NULL)
                    {
                        free(source.direc);
                        source.direc = NULL;
                    }
                    return -1;
                }
                // Free allocated resources
                if (source.direc != NULL)
                    free(source.direc);
                source.direc = NULL;
                return 1; // return success
            }
        }
        // error message
        printf("No space left in Destination Directory. Failed to move source\n");
    }
    return -1; // return failue if we get here
}
// funcation to load a directory given the corresponding directory entry
DE *loadDir(DE toBeLoaded)
{
    // Calculate the number of blocks needed to read
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;
    // Allocate memory to hold the directory array
    DE *returnValue = malloc(numBlocks);

    // check if memory allocation failed
    if (returnValue == NULL)
    {
        puts("malloc failed in loadDir");
        return NULL;
    }
    // read the directory entries from disk intro the allocated memory
    if (LBAread(returnValue, numBlocks, toBeLoaded.loc) != numBlocks)
    {
        puts("loadDir failed");
        free(returnValue);
        return NULL;
    }
    // Return the pointer to the successfully loaded directory array
    return returnValue;
}