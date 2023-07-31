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

// allocate memory for the extern cwdString
char *cwdString = "/";
DE *cwdPointer;
// given a file path, returns whether the path is valid
// to be valid, all directories to the right must be immediate children
// of the directory immediaely to their left.

// indicates through status
// whether the path is valid or not
// whether the last path exists
// also returns a pointer to the second to last directoryEntry
// if path is invalid (if status==0) DO NOT USE THE returned pointer
// if status==1 path is valid and last part of path does NOT exist
// if status==2 path is valid and last part of path DOES exist
PathReturn parsePath(char *path)
{

    // printf("the path being parsed is: %s\n", path);
    // wrapper struct to be returned
    PathReturn ret;
    ret.index_last = 0;
    // 2 cases:
    // 1. path is an absolute path
    // 2. path is a relative path
    // determine which case we are in
    DE *root = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1);
    if (root == NULL)
    {
        printf("malloc failed in parsePath\n");
        ret.status_code = -1;
        return ret;
    }
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;
    if (path[0] == '/')
    {
        // path is an absolute path so start at root
        LBAread(root, numBlocks, vcb->root_index);
    }
    else
    {
        // path is relative so start at cwd
        LBAread(root, numBlocks, cwdPointer[0].loc); // TODO: make sure te wcdPointer is actually initialized
    }
    ret.direc = root; // set the starting dir in our result struct
    // get the first child dir.
    char *currentToken = strtok(path, "/");
    // found flag indicates if child directory was found in parent
    int found;
    while (currentToken != NULL)
    {              // loop through the whole path DE by DE
        found = 0; // haven't found the child DE yet
        // look thorugh current directory for child
        for (int i = 0; i < BUFFER_SIZE; ++i)
        {
            if (strcmp(ret.direc[i].name, currentToken) == 0)
            {
                // child was found and so far the path is valid
                found = 1;
                // setup to replace parent with found child
                DE *nextDir = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1);
                if (nextDir == NULL)
                {
                    printf("malloc failed in parsepath function\n");
                    ret.status_code = -1;
                    return ret;
                }
                if (ret.direc[i].isDirectory == 0)
                {
                    // the current token is a file so path is invalid
                    // if it is not the last token
                    currentToken = strtok(NULL, "/");
                    // check if the file is the last token
                    if (currentToken == NULL)
                    {
                        // the file was the last token so path is valid
                        // and the file exists
                        ret.status_code = 2;
                        ret.index_last = i;
                        return ret;
                    }
                    // file was NOT the last token so path is invalid
                    ret.status_code = 0;
                    return ret;
                } // child is a valid directory in parent
                // so set parent to child for next iteration
                LBAread(nextDir, numBlocks, ret.direc[i].loc);
                ret.direc = nextDir;
                ret.index_last = i;
                // since path was found go to next iteration
                break;
            }
        }
        if (found != 1)
        {
            // did not find child dir/file in current directory
            // path is invalid if child dir/file is not last path
            currentToken = strtok(NULL, "/");
            // check if child is last DE in path
            if (currentToken == NULL)
            {
                // current token was the last path
                // so path IS valid
                // the directory/file does NOT exist yet
                ret.status_code = 1;
                return ret;
            }
            // current token was not found
            // and was not the last DE in path so path is invalid
            ret.status_code = 0;
            return ret;
        }
        // move on to next token
        currentToken = strtok(NULL, "/");
    }
    // every DE in the path is valid and exists
    // so return the parent of the last entry
    ret.status_code = 2;
    // change to parent of last entry

    if (LBAread(ret.direc, numBlocks, ret.direc[1].loc) != numBlocks)
    {
        printf("LBAread failed in parsePath \n");
        ret.status_code = -10;
    }
    return ret;
}

// returns the current work directory that the user is in
char *fs_getcwd(char *pathname, size_t size)
{
    // printf("This is the cwdString: %s", cwdString);
    return cwdString;
}
//returns the canonical path of an absolute path
char *getCanonicalPath(char *path)
{
    // will hold the canonical path
    char newPath[1000] = {};
    // for processing input path
    char *stack[strlen(path)];
    // for strtok_r
    char *saveptr;
    // keeps track of where we are in stack
    int top = -1;

    // fill stack with tokens from path
    char *token = strtok_r(path, "/", &saveptr);

    while (token != NULL)
    {
        if (strcmp(token, "..") == 0)
        {
            // if the stack isn't empty
            if (top > -1)
            {
                // pop last element off
                top--;
            }
        }
        else if (strcmp(token, ".") == 0)
        {
            // ignore this case as . does nothing
        }
        else
        {
            // current token is an actual element
            // so push it to the stack
            stack[++top] = token;
        }
        // get next token
        token = strtok_r(NULL, "/", &saveptr);
    }

    // create string out of stack.
    newPath[0] = '/';
    for (int i = 0; i <= top; ++i)
    { // loop through stack
        // and add every element in the stack to the string
        strcat(newPath, stack[i]);
        // if we aren't at the final element
        if (i != top)
        {

            // add a slash in between elements
            strcat(newPath, "/");
        }
        // if we are at the final element
        else
        {

            // add a null terminator
            strcat(newPath, "\0");
        }
    }
    // convert newPath to a char*
    char *result = (char *)malloc(strlen(newPath) * sizeof(char) + 1);
    strcpy(result, newPath);

    return result;
}

// linux chdir
int fs_setcwd(char *pathname)
{
    // since parsePath will mutate pathname
    // create a copy of pathname
    char *copyOfPath = malloc(strlen(pathname) + 1);
    strcpy(copyOfPath, pathname);

    PathReturn res;
    res = parsePath(pathname);

    if (res.status_code == -1)
    {
        printf("parsePath failed in fs_setcwd\n");
        if (res.direc != NULL)
            free(res.direc);
        res.direc = NULL;
        free(copyOfPath);
        return -1;
    }
    if (res.status_code != 2)
    {
        printf("Invalid path %s\n", pathname);
        free(copyOfPath);
        if (res.direc != NULL)
            free(res.direc);
        res.direc = NULL;
        return -1;
    }
    // if path points to file, command is invalid
    if (res.direc[res.index_last].isDirectory != 1)
    {
        // path points to file and command is invalid
        printf("Path %s is invalid as it points to file \n", pathname);
        free(copyOfPath);
        if (res.direc != NULL)
            free(res.direc);
        res.direc = NULL;
        return -1;
    }
    // path is valid so change directory

    // case 1: pathname is relative
    // case 2: pathname is absolute
    if (copyOfPath[0] != '/')
    {
        // pathname is relative
        // so concatenate the path to the cwdString
        // create new string of proper size to hold everything
        char newCwdString[strlen(cwdString) + 2 + strlen(copyOfPath)];
        // copy old cwd into the first part of new string
        strcpy(newCwdString, cwdString);
        // concatenate a /
        strcat(newCwdString, "/");
        // concatenate the relative new path
        strcat(newCwdString, copyOfPath);
        // convert resultant path to a canonical path
        // set cwdString to new path
        cwdString = getCanonicalPath(newCwdString);
    }
    else
    {
        printf("path is %s\n", copyOfPath);

        // path is absolute
        // convert pathname to canonical path
        //  set the cwdString to the new pathname
        cwdString = getCanonicalPath(copyOfPath); // may need to free old cwdString
    }
    // free cwdPointer's old content
    // free(cwdPointer); // might not be necessary and might need to be removed if cwdPointer was never malloced
    // set cwdPointer
    int cwdBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;
    if (LBAread(cwdPointer, cwdBlocks, res.direc[res.index_last].loc) != cwdBlocks)
    {
        printf("LBAread failed in delete \n");
    }
    free(copyOfPath);
    if (res.direc != NULL)
        free(res.direc);
    res.direc = NULL;
    return 0;
}
// creates new directory. returns 1 upon success -1 on failure.
int fs_mkdir(const char *pathname, mode_t mode)
{
    // get started index of last elemnt in path
    // Ex.:If path a home/Desktop/newFolder
    // We need "newFolder"
    char copy[strlen(pathname)];

    strcpy(copy, pathname);
    int index = 0;
    int indexParent;
    for (int i = strlen(copy); i >= 0; i--)
    {
        if (copy[i] == '/')
        {
            index = ++i;
            break;
        }
    }

    PathReturn parent;
    // call parse path
    parent = parsePath((char *)pathname);
    // check the returned results of parsepath
    if (parent.status_code != 1)
    {
        printf("The path you gave is invalid: %s\n", copy);
        if (parent.status_code == 2)
        {
            printf("The directory: %s already exists\n", pathname + index);
        }
        if (parent.direc != NULL)
            free(parent.direc);
        parent.direc = NULL;
        return -1;
    }
    // get the name of the directory to be created from the path passed in
    char newDirectoryName[strlen(copy) - index + 1];
    strncpy(newDirectoryName, copy + index, strlen(copy) - index + 1);
    // printf("inside md: new directory name: %s parent is: %s \n", newDirectoryName, parent.directory);

    if (strcmp(newDirectoryName, parent.direc[0].name) == 0)
    {
        // if the name of the dir to be created is the same as the parent
        // then rudely deny the user's request to create the new dir
        printf("You think you can create a directory named the same as the parent??\n");
        printf("No. You can't-- directory %s not created\n", newDirectoryName);
        return -1;
    }
    // check of there is space in the parent directory
    for (int i = 2; i < BUFFER_SIZE; i++)
    {
        if (strlen(parent.direc[i].name) == 0)
        {
            // allocate the blocks needed for new dir on disk and update the bitmap
            int newLocation = initDir(parent.direc, vcb->block_size);
            if (newLocation == -1)
            {
                printf("error occurred in initDir in mkDir function \n");
                if (parent.direc != NULL)
                    free(parent.direc);
                parent.direc = NULL;
                return -1;
            }
            DE newDirectory;
            // Initialize new directory values
            newDirectory.isDirectory = 1;
            newDirectory.created_at = time(NULL);
            newDirectory.last_accessed = time(NULL);
            newDirectory.modified_at = time(NULL);
            newDirectory.loc = newLocation;
            // set name
            strncpy(newDirectory.name, newDirectoryName, sizeof(newDirectory.name));
            parent.direc[i] = newDirectory;
            // write the updated parent to disk
            int parentBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;
            LBAwrite(parent.direc, parentBlocks, parent.direc->loc);
            if (parent.direc != NULL)
                free(parent.direc);
            parent.direc = NULL;
            return 1;
        }
    }
    // If we reach this point, then there is no space available for
    // new directory
    printf("Parent directory has no space for new directory\n");
    if (parent.direc != NULL)
        free(parent.direc);
    parent.direc = NULL;
    return -1;
}
// if result is -1 path is invalid. If result is 1 then isDir if result is 0 then it is file.
int fs_isDir(char *pathname)
{
    // call parsePath on the path
    PathReturn result = parsePath(pathname);
    // analyze the returned value of parsePath
    if (result.status_code == -1)
    {
        printf("ParsePath failed\n");

        return -1;
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
    }
    else if (result.status_code < 2)
    {
        printf("Path is Invalid\n");
        if (result.direc!= NULL)
            free(result.direc);
        result.direc= NULL;
        return -1;
    }
    // path is valid
    // we have pointer to parent but we want actual element
    // get actual element
    // return whether the actual element is a directory
    int res = result.direc[result.index_last].isDirectory;
    if (result.direc != NULL)
        free(result.direc);
    result.direc = NULL;
    return res;
}
// if result is -1 path is invalid. If result is 1 then isFile if result is 0 then it is dir.
int fs_isFile(char *filename)
{
    int res = fs_isDir(filename);
    if (res < 0)
    {
        return res;
    }
    return (res == 1) ? 0 : 1;
}

fdDir *fs_opendir(const char *pathname)
{

    // call parsePath on the path
    PathReturn result = parsePath((char *)pathname);
    // analyze the returned value of parsePath
    if (result.status_code == -1)
    {
        printf("ParsePath failed\n");
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return NULL;
    }
    else if (result.status_code < 2)
    {
        printf("Path is Invalid\n");
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return NULL;
    }
    // path is valid

    struct fs_stat *stat = malloc(sizeof(struct fs_stat));
    if (stat == NULL)
    {
        printf("malloc failed in fs_opendir. Did not open dir.\n");
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return NULL;
    }
    fs_stat(pathname, stat);
    fdDir *dir = malloc(sizeof(fdDir));
    if (dir == NULL)
    {
        printf("error in malloc occurred in fs_opendir function\n");
    }

    dir->d_reclen = stat->st_size;
    dir->directoryStartLocation = result.direc[result.index_last].loc;
    dir->dirEntryPosition = 0;
    dir->directory = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1);
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;
    if (LBAread(dir->directory, numBlocks, dir->directoryStartLocation) != numBlocks)
    {
        printf("LBAread failed in opendir function\n");
        if (result.direc!= NULL)
            free(result.direc);
        result.direc = NULL;
        return NULL;
    }
    if (result.direc != NULL)
        free(result.direc);
    result.direc = NULL;
    return dir;
}
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    // check if the arg is valid
    if (dirp == NULL)
    {
        printf("the directory Item is not defined.\n");
        return NULL;
    }
    if (dirp->dirEntryPosition > BUFFER_SIZE)
    {
        // reached end of directory.
        return NULL;
    }
    // declare struct to be returned
    struct fs_diriteminfo *result = malloc(sizeof(struct fs_diriteminfo));
    // initialize the struct
    result->d_reclen = sizeof(struct fs_diriteminfo);
    // set the fileType. It could either be a dir or a file.

    result->fileType = (dirp->directory[dirp->dirEntryPosition].isDirectory == 1) ? FT_DIRECTORY : FT_REGFILE;
    // set the name of the directoryItem
    strncpy(result->d_name, dirp->directory[dirp->dirEntryPosition].name, sizeof(result->d_name));
    // since we just read the current dirItem, move to the next dirItem
    int i = dirp->dirEntryPosition;
    char *currName = dirp->directory[++dirp->dirEntryPosition].name;
    while (strlen(currName) == 0 && (i < BUFFER_SIZE))
    {
        dirp->dirEntryPosition++;
        i++;
        currName = dirp->directory[dirp->dirEntryPosition].name;
    }

    return result;
}

int fs_closedir(fdDir *dirp)
{
    // free the fsStat struct
    free(dirp->directory); // TODO: if valgrind raises issues, create for loop to free every item in the array
    dirp->directory = NULL;
    // free the dirp instance
    free(dirp);
    return 1;
}
// returns 1 upon success and 0 upon failure
// must be in the immediate parent of the file.
// if filename is valid then delete the file
int fs_delete(char *filename)
{ // removes a file
    // reload the cwdpointer
    int blocksForcwd = (BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
    if (LBAread(cwdPointer, blocksForcwd, cwdPointer[0].loc) != blocksForcwd)
    {
        puts("LBAread failed in fs_delete");
        return 0;
    }
    // locate the file in the cwd
    for (int i = 2; i < BUFFER_SIZE; ++i)
    {
        if (strcmp(cwdPointer[i].name, filename) == 0)
        {
            // file found so delete the file
            // free the blocks by indicating which blocks are free via the bitmap
            for (int currentExt = 0; currentExt < 8; currentExt++)
            {
                if (cwdPointer[i].extents[currentExt].contiguous_blocks != 0)
                    deallocateBlocks(cwdPointer[i].extents[currentExt].contiguous_blocks, cwdPointer[i].extents[currentExt].block_number);
            }
            // deallocate the blocks for the  DE for the file as well
            int fileDEBlocks = (sizeof(DE) + vcb->block_size - 1) / vcb->block_size;
            deallocateBlocks(fileDEBlocks, cwdPointer[i].loc);

            // remove the file from the parent directory
            // by changing its name to NULL
            cwdPointer[i].name[0] = '\0';
            // write the modified parent back to disk.
            if (LBAwrite(cwdPointer, (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size, cwdPointer[0].loc) !=
                (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size)
            {
                printf("LBAwrite failed in fs_delete. File deallocated but still remains inside parent folder.\n");
                return -1;
            }
            // since deletion was successful…

            return 1;
        }
    }
    // if we reach this point in the function body, then file not found in parent
    // and so we did NOT delete the file
    printf("file: %s was not found in cwd: %s File was not deleted.\n", filename, cwdString);
    return 0;
}
// remove a folder
// or a file?
int fs_rmdir(const char *pathname)
{
    // parse the path
    PathReturn result = parsePath((char *)pathname);
    // if the last element in pathname does not exist or path is invalid
    if (result.status_code != 2)
    {
        if (result.direc != NULL)
            free(result.direc);
        result.direc = NULL;
        return -1;
    }
    DE *toBeRemoved = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1);
    if (toBeRemoved == NULL)
    {
        printf("malloc failed in rmdir\n");
        if (result.direc != NULL)
        {
            free(result.direc);
            result.direc = NULL;
        }
        return -1;
    }
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;

    LBAread(toBeRemoved, numBlocks, result.direc[result.index_last].loc);

    if (result.direc[result.index_last].isDirectory == 0)
    {
        fs_delete(result.direc[result.index_last].name);
    }

    // path is legal. Check if folder is not empty
    for (int i = 2; i < BUFFER_SIZE; ++i)
    {
        // if the name is not empty
        if (strlen(toBeRemoved[i].name) != 0)
        {
            // found child in directory
            // so directory is NOT empty
            printf("Directory is NOT empty... cannot remove directory\n");
            if (result.direc != NULL)
            {
                free(result.direc);
                result.direc = NULL;
            }
            return -1;
        }
    }
    // if reached this point, directory is empty
    // so remove the directory
    // update the freespace structure to reflect
    //  that the removed directory’s blocks are now free
    deallocateBlocks(numBlocks, result.direc[result.index_last].loc);
    // delete the directory’s DE in the parent
    // by setting the name to empty string
    result.direc[result.index_last].name[0] = '\0';
    // write the updated parent to disk
    if (LBAwrite(result.direc, numBlocks, result.direc->loc) != numBlocks)
    {
        printf("LBAwrite failed in rmdir function\n");
    }
    if (result.direc != NULL)
        free(result.direc);
    result.direc = NULL;
    return 1;
}

// returns 1 on success 0 on failure
// populates buf with info on the path arg
int fs_stat(const char *path, struct fs_stat *buf)
{
    // parsePath to see if it is legal
    // if the last element does NOT exist then path is illegal
    // if path is invalid path is illegal
    PathReturn ret = parsePath((char *)path);
    if (ret.status_code != 2)
    {
        printf("path: %s is not valid. The buffer has NOT been populated\n", path);
        if (ret.direc != NULL)
            free(ret.direc);
        ret.direc = NULL;
        return 0;
    }
    // path is legal

    DE lastElement = ret.direc[ret.index_last];
    buf->st_size = (ret.direc[ret.index_last].isDirectory == 1) ? BUFFER_SIZE * sizeof(DE) + vcb->block_size - 1 : lastElement.size; // TODO: change third ternary operand so that it calcs the size of the file using extents.
    // buf->st_size = lastElement.size;
    buf->st_blksize = vcb->block_size;
    buf->st_blocks = (lastElement.size + vcb->block_size - 1) / vcb->block_size;
    buf->st_accesstime = lastElement.last_accessed || (time_t)0;
    buf->st_modtime = lastElement.modified_at || (time_t)0;
    buf->st_createtime = lastElement.created_at || (time_t)0;
    if (ret.direc != NULL)
        free(ret.direc);
    ret.direc = NULL;
    return 1;
}

// if dest exists then it must be a dir and then move src into dest
// else rename src to dest
// dest must either be a valid path to dest or a name  or a file
// src must be a valid path and exist and it can either be a file or dir
// returns 1 on success, -1 on failure
int fs_mv(char *srcPath, char *destPath)
{

    // check src for validity
    PathReturn srcResult = parsePath(srcPath);
    // path is not valid or the element at the end does not exist
    if (srcResult.status_code != 2)
    {
        printf("invalid path given\n");
        if (srcResult.direc != NULL)
            free(srcResult.direc);
        srcResult.direc = NULL;
        return -1;
    }
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1) / vcb->block_size;
    PathReturn destResult = parsePath(destPath);
    // if path is valid but last element in dest does not exist

    if (destResult.status_code == 1)
    {
        printf("changing name\n");
        int nameLen = sizeof(srcResult.direc[srcResult.index_last].name);
        // then rename the src to destPath
        strncpy(srcResult.direc[srcResult.index_last].name, destPath, nameLen);

        if (LBAwrite(srcResult.direc, numBlocks, srcResult.direc[0].loc) != numBlocks)
        {
            printf("LBAwrite failed in mv function\n");
            if (srcResult.direc != NULL)
                free(srcResult.direc);
            srcResult.direc = NULL;
            return -1;
        }
        if (destResult.direc != NULL)
            free(destResult.direc);
        destResult.direc = NULL;
        if (srcResult.direc != NULL)
            free(srcResult.direc);
        srcResult.direc = NULL;
        return 1;
    }
    if (destResult.status_code == 2)
    {
        // the dest does exist but is it a dir?
        if (destResult.direc[destResult.index_last].isDirectory != 1)
        {
            if (srcResult.direc[srcResult.index_last].isDirectory == 1)
            {
                printf("cannot move a directory into a file.\n");
                if (srcResult.direc != NULL)
                    free(srcResult.direc);
                srcResult.direc = NULL;
                return -1;
            }
            // move file into file
            // same as moving a dir

            // change the DE in dest array to the src file DE
            destResult.direc[destResult.index_last] = srcResult.direc[srcResult.index_last];
            // delete src in its original parent
            srcResult.direc[srcResult.index_last].name[0] = '\0';
            // write the modified directory Arrays back to disk
            if (LBAwrite(destResult.direc, numBlocks, destResult.direc[0].loc) != numBlocks)
            {
                puts("LBAwrite failed in fs_mv");
                return -1;
            }
            if (LBAwrite(srcResult.direc, numBlocks, srcResult.direc[0].loc) != numBlocks)
            {
                puts("LBAwrite failed in fs_mv");
                return -1;
            }
            return 1;
        }
        // if we made it this far destPath is a valid path to a directory
        // grab the path to the src
        char *pathToSrc = getCanonicalPath(srcPath);
        // so move srcPath into destPath
        // load the destination directory array into mem from disk
        DE *destinationDir = malloc(sizeof(DE) * BUFFER_SIZE + vcb->block_size - 1);
        if (destinationDir == NULL)
        {
            printf("malloc failed in fs_mv function\n");
            if (srcResult.direc != NULL)
                free(srcResult.direc);
            srcResult.direc = NULL;
            return -1;
        }

        if (LBAread(destinationDir, numBlocks, destResult.direc[destResult.index_last].loc) != numBlocks)
        {
            printf("LBAread failed in fs_mv function\n");
            if (srcResult.direc != NULL)
                free(srcResult.direc);
            srcResult.direc = NULL;
            return -1;
        }
        for (int i = 2; i < BUFFER_SIZE; ++i)
        {
            if (strlen(destinationDir[i].name) == 0)
            {
                // found a free space for src DirectoryEntry
                // copy src DirectoryEntry into the free space
                printf("the name of the dir to move is: %s and it its isDir is: %d\n", srcResult.direc[srcResult.index_last].name, srcResult.direc[srcResult.index_last].isDirectory);
                // move src into dest
                destinationDir[i] = srcResult.direc[srcResult.index_last];
                // commit changed parent  to disk
                if (LBAwrite(destinationDir, numBlocks, destinationDir[0].loc) != numBlocks)
                {
                    printf("error in LBAwrite in fs_mv function\n");
                    if (srcResult.direc != NULL)
                    {
                        free(srcResult.direc);
                        srcResult.direc = NULL;
                    }
                    return -1;
                }
                // delete the src DirectoryEntry from the parent of the src directory
                srcResult.direc[srcResult.index_last].name[0] = '\0';
                // write the modified directories to disk
                if (LBAwrite(srcResult.direc, numBlocks, srcResult.direc[0].loc) != numBlocks)
                {
                    printf("error in LBAwrite in fs_mv function\n");
                    if (srcResult.direc != NULL)
                    {
                        free(srcResult.direc);
                        srcResult.direc = NULL;
                    }
                    return -1;
                }
                // free resources.
                if (srcResult.direc != NULL)
                    free(srcResult.direc);
                srcResult.direc = NULL;
                return 1;
            }
        }
        // if we are here then there was no space left in the destination directory
        printf("No space left in Destination Directory. Did not move source\n");
    }
    return -1;
}
// loads a directory array given the corresponding directory Entry.
DE *loadDir(DE toBeLoaded)
{
    int numBlocks = (sizeof(DE) * BUFFER_SIZE + vcb->block_size- 1) / vcb->block_size;
    DE *ret = malloc(numBlocks);
    if (ret == NULL)
    {
        puts("malloc failed in loadDir");
        return NULL;
    }
    if (LBAread(ret, numBlocks, toBeLoaded.loc) != numBlocks)
    {
        puts("loadDir failed");
        free(ret);
        return NULL;
    }
    return ret;
}