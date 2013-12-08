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
    struct inode* Inode;
    read_inode(rd, InodeNO, Inode);
    if (Inode->type == 1)
        return Inode->size; // return the size of the file 
    else
        return(-1); // -1 means there is no such file or the file is a directory file
}

int create_file (uint8_t* rd, int ParentInodeNO, char* name)
{
    if (strlen(name) >= 14) {
#ifdef UL_DEBUG
        printf("File name is toooooo long!\n");
#endif
        return(-1);
    }
    struct superblock* SuperBlock;
    struct inode* ParentInode;
    struct inode* Inode;
    int InodeNO;
    int blockNO;
    uint8_t entry_pos;

    read_superblock(rd, SuperBlock);
    if (SuperBlock->FreeBlockNum != 0 && SuperBlock->FreeInodeNum != 0)
    {
        InodeNO = find_next_free_inode(rd);
        Inode->type = (uint8_t) 0x01;
        Inode->size = (uint32_t) 0x0;
        update_inode(rd, InodeNO, Inode);
        read_inode(rd, ParentInodeNO, ParentInode);
        double temp = ParentInode->size/BLOCK_SIZE;
        if (temp - (int)temp > 0)
            blockNO = (int)temp+1;
        else if (temp - (int)temp < 0)
            blockNO = (int)temp;
        else {
            blockNO = (int)temp+1;
            ParentInode->BlockPointer[blockNO] = rd+find_next_free_block*BLOCK_SIZE;
        }
        entry_pos=ParentInode->size%BLOCK_SIZE;  
        strcpy(rd[ParentInode->BlockPointer[blockNO]+entry_pos], name);
        *rd[ParentInode->BlockPointer[blockNO]+entry_pos+15] = InodeNO;
        ParentInode->size += 16;
        update_inode(rd, ParentInodeNO, ParentInode);
    }    
    else
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

