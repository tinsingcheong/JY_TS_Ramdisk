#include "constant.h"
#include "user_struct.h"

#define UL_DEBUG

#ifdef UL_DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

int write_file (uint8_t* rd, int InodeNO, int pos, char* string, int length)
{
    
    return(-1); // -1 means nothing is written into the file 
}

int read_file (uint8_t* rd, int InodeNO, int pos)
{
    return(-1); // -1 means nothing is read from the file
}

int read_dir (uint8_t* rd, int InodeNO)
{
    return(-1); // -1 means no such directory
}

int get_file_size (uint8_t* rd, int InodeNO)
{
    return(-1); // -1 means there is no such file or the file is a directory file
}

int create_file (uint8_t* rd, int ParentDirInode, char* name)
{
    return(-1); // -1 means creation fails
}

int create_dir (uint8_t* rd, int ParentDirInode, char* name)
{
    return(-1); // -1 means creation fails
}

int remove_file (uint8_t* rd, int InodeNO, char* name)
{
    return(-1); // -1 means removal fails
}

int remove_dir (uint8_t* rd, int InodeNO, char* name)
{
    return(-1); // -1 means removal fails
}

