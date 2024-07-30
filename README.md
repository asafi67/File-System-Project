# CSC415 Group Term Assignment - File System

To format the README file for GitHub, you need to use Markdown syntax. Here's the complete README file formatted using Markdown:

```markdown
# Custom File System - File System Soldiers

## Overview

This project is a custom block-based file system designed and implemented by the File System Soldiers. The file system provides basic file and directory management functionalities, efficient block allocation using a bitmap, and extent-based storage allocation. This README provides an overview of the components and instructions for building and running the file system.

## Team Members

- Anish Khadka
- Joe Sand
- Ameen Safi

## Project Structure

The project consists of several files that together implement the file system:

### Source Files

- `b_io.c` / `b_io.h`: Buffered I/O operations, providing high-level file read and write functions.
- `bitmap.c` / `bitmap.h`: Bitmap management for block allocation and deallocation.
- `extent.h`: Definitions for extent-based allocation.
- `fsLow.c` / `fsLow.h`: Low-level disk interactions using logical block addressing (LBA).
- `mfs.c` / `mfs.h`: Main file system functions for managing files and directories.
- `rootDir.c` / `rootDir.h`: Root directory management, handling directory entries.
- `volumeCB.c` / `volumeCB.h`: Volume Control Block (VCB) for storing file system metadata.
- `fsshell.c`: Shell interface for interacting with the file system via command-line commands.
- `Makefile`: Build instructions for compiling the project.

## Features

- **Block-Based Storage**: Fixed-size blocks managed by a bitmap.
- **Bitmap Management**: Efficient tracking of free and allocated blocks.
- **Extent-Based Allocation**: Contiguous blocks of storage for efficient file storage.
- **Buffered I/O Operations**: Improved performance for read and write operations.
- **Low-Level Disk Access**: Direct interaction with the disk using logical block addressing.
- **Root Directory Management**: Hierarchical directory structure with support for file and directory creation and deletion.
- **Shell Interface**: Command-line tools for file system operations.

## Building the Project

To build the project, use the provided `Makefile`. Ensure you have `make` installed on your system.

```sh
make
```

This command will compile all source files and create the necessary binaries.

## Running the File System

After building the project, you can run the file system shell to interact with the file system.

```sh
./fsshell
```

The shell provides various commands for file system operations, such as creating files and directories, reading and writing files, and deleting files and directories.

## File System Commands

Here are some of the commands you can use in the file system shell:

- `create <filename>`: Create a new file.
- `delete <filename>`: Delete an existing file.
- `read <filename>`: Read the contents of a file.
- `write <filename> <data>`: Write data to a file.
- `mkdir <dirname>`: Create a new directory.
- `rmdir <dirname>`: Remove an existing directory.
- `ls`: List files and directories in the current directory.
- `cd <dirname>`: Change the current directory.

## Example Usage

```sh
./fsshell
> mkdir test
> cd test
> create file1.txt
> write file1.txt "Hello, world!"
> read file1.txt
> ls
> delete file1.txt
> cd ..
> rmdir test
```

## License

This project is licensed under the MIT License.

## Acknowledgements

This project was developed as part of the CSC-415 course at San Francisco State University. Special thanks to our instructor and teaching assistants for their guidance and support.



Copy and paste this Markdown content into your `README.md` file in your GitHub repository. This will ensure it is properly formatted and easy to read on GitHub.

