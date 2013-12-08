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

void create_file (uint8_t* rd, int ParentInodeNO, char* name)
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
    uint16_t InodeNO;
    int blockNO;
    int new_block_id;
    uint8_t entry_pos;
    
	if(!(SuperBlock=(struct superblock*)malloc(sizeof(struct superblock)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(Inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentInode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
    read_superblock(rd, SuperBlock);
    if (SuperBlock->FreeBlockNum != 0 && SuperBlock->FreeInodeNum != 0)
    {
        InodeNO = find_next_free_inode(rd);
        set_inode_bitmap(rd, InodeNO);
        Inode->type = (uint8_t) 0x01; // Initialize the type as regular file
        Inode->size = (uint32_t) 0x0; // Initialize the size as 0
        update_inode(rd, InodeNO, Inode); // Update the new Inode
        // Update the Parent Inode information
        read_inode(rd, ParentInodeNO, ParentInode);
        double temp = ParentInode->size/BLOCK_SIZE;
        if (temp - (int)temp > 0) // if the current blocks are not filled up
            blockNO = (int)temp;
        else if (temp-(int)temp == 0) { // if the current blocks are filled up
            blockNO = (int)temp+1;
            new_block_id = find_next_free_block(rd);
            set_bitmap(rd, new_block_id);
            if (blockNO<=7 && blockNO>=0) 
                ParentInode->BlockPointer[blockNO] = rd+new_block_id*BLOCK_SIZE;
            else if (blockNO > 7 && blockNO <= 7+64)
                *(ParentInode->BlockPointer[8]+blockNO-8) = rd+new_block_id*BLOCK_SIZE;
            else if (blockNO > 7+64 && blockNO <= 7+64+64*64)
                *(*(ParentInode->BlockPointer[9])+blockNO-8-64) = rd+new_block_id*BLOCK_SIZE;
            else{
#ifdef UL_DEBUG
                printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
            }
        }

        entry_pos=ParentInode->size%BLOCK_SIZE; // the position that the entry should be written to 
        if (blockNO<=7 && blockNO>=0) {
            strcpy(rd[ParentInode->BlockPointer[blockNO]+entry_pos], name);
            *rd[ParentInode->BlockPointer[blockNO]+entry_pos+15] = InodeNO;
        }
        else if (blockNO>7 && blockNO<=7+64) {
            strcpy(rd[*(ParentInode->BlockPointer[8]+blockNO-8)+entry_pos],name);
            *rd[*(ParentInode->BlockPointer[8]+blockNO-8)+entry_pos+15]=InodeNO;
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
            strcpy(rd[*(*(ParentInode->BlockPointer[9])+blockNO-8)+entry_pos],name);
            *rd[*(*(ParentInode->BlockPointer[9])+blockNO-8)+entry_pos+15]=InodeNO;
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
            
        ParentInode->size += 16;
        update_inode(rd, ParentInodeNO, ParentInode);
    }    
    else
        return(-1); // -1 means creation fails
}

void create_dir (uint8_t* rd, int ParentDirInode, char* name)
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
    uint16_t InodeNO;
    int blockNO;
    int new_block_id;
    uint8_t entry_pos;
    
	if(!(SuperBlock=(struct superblock*)malloc(sizeof(struct superblock)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(Inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentInode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
    read_superblock(rd, SuperBlock);
    if (SuperBlock->FreeBlockNum != 0 && SuperBlock->FreeInodeNum != 0)
    {
        InodeNO = find_next_free_inode(rd);
        set_inode_bitmap(rd, InodeNO);
        Inode->type = (uint8_t) 0x0;
        Inode->size = (uint32_t) 0x0;
        update_inode(rd, InodeNO, Inode);
        read_inode(rd, ParentInodeNO, ParentInode);
        double temp = ParentInode->size/BLOCK_SIZE;
        if (temp - (int)temp > 0)
            blockNO = (int)temp;
        else {
            blockNO = (int)temp+1;
            new_block_id = find_next_free_block(rd);
            set_bitmap(rd, blockNO);
            if (blockNO<=7 && blockNO>=0) 
                ParentInode->BlockPointer[blockNO] = rd+new_block_id*BLOCK_SIZE;
            else if (blockNO > 7 && blockNO <= 7+64)
                *(ParentInode->BlockPointer[8]+blockNO-8) = rd+new_block_id*BLOCK_SIZE;
            else if (blockNO > 7+64 && blockNO <= 7+64+64*64)
                *(*(ParentInode->BlockPointer[9])+blockNO-8-64) = rd+new_block_id*BLOCK_SIZE;
            else{
#ifdef UL_DEBUG
                printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
            }
        }

        entry_pos=ParentInode->size%BLOCK_SIZE;  
        if (blockNO<=7 && blockNO>=0) {
            strcpy(rd[ParentInode->BlockPointer[blockNO]+entry_pos], name);
            *rd[ParentInode->BlockPointer[blockNO]+entry_pos+15] = InodeNO;
        }
        else if (blockNO>7 && blockNO<=7+64) {
            strcpy(rd[*(ParentInode->BlockPointer[8]+blockNO-8)+entry_pos],name);
            *rd[*(ParentInode->BlockPointer[8]+blockNO-8)+entry_pos+15]=InodeNO;
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
            strcpy(rd[*(*(ParentInode->BlockPointer[9])+blockNO-8)+entry_pos],name);
            *rd[*(*(ParentInode->BlockPointer[9])+blockNO-8)+entry_pos+15]=InodeNO;
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
        ParentInode->size += 16;
        update_inode(rd, ParentInodeNO, ParentInode);
    }    
    else
        exit(-1); // -1 means creation fails
}

void remove_file (uint8_t* rd, int ParentInodeNO, int InodeNO, char* name)
{
    if (strlen(name) >= 14) {
#ifdef UL_DEBUG
        printf("File name is toooooo long!\n");
#endif
        return(-1);
    }
    struct inode* ParentInode;
    struct inode* Inode;
    int blockNO;
    uint8_t entry_pos;
    int i;

	if(!(SuperBlock=(struct superblock*)malloc(sizeof(struct superblock)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(Inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentInode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}

    read_inode(rd, ParentInodeNO, ParentInode);
    read_inode(rd, InodeNO, Inode);
    clr_inode_bitmap(rd, InodeNO);
    
    double temp = Inode->size/BLOCK_SIZE;
    if (temp - (int)temp > 0)
        blockNO = (int)temp+1;
    else 
        blockNO = (int)temp;

    for (i=0;i<blockNO;i++)
        set_bitmap(rd, (Inode->BlockPointer[blockNO]-rd)/BLOCK_SIZE);

    should strcmp with all the entries in ParentInode and delete the corresponding one.
}

void remove_dir (uint8_t* rd, int ParentInodeNO, int InodeNO, char* name)
{
    return(-1); // -1 means removal fails
}

void switch_block (uint8_t* rd, int DirInodeNO, char)
