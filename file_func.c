#include "constant.h"
#include "user_struct.h"
#include "ramdisk.h"

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
    return Inode->size; // return the size of the file 
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
    struct dir_entry* NewDirEntry;
    uint16_t InodeNO;
    int blockNO;
    int new_block_id;
    int new_entry_block_id;
    int new_entry_table_block_id;
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
	if(!(NewDirEntry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
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
                ParentInode->BlockPointer[blockNO] = new_block_id;
            else if (blockNO > 7 && blockNO <= 7+64){
                if (blockNO == 8) {
                    new_entry_block_id = find_next_free_block(rd); 
                    set_bitmap(rd, new_entry_block_id);
                    ParentInode->BlockPointer[8]=rd+new_entry_block_id*BLOCK_SIZE;
                }
                *(ParentInode->BlockPointer[8]+(blockNO-8)*4) = (uint8_t)(new_block_id & 0x000000ff);
                *(ParentInode->BlockPointer[8]+(blockNO-8)*4+1) = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                *(ParentInode->BlockPointer[8]+(blockNO-8)*4+2) = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                *(ParentInode->BlockPointer[8]+(blockNO-8)*4+3) = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
            }
            else if (blockNO > 7+64 && blockNO <= 7+64+64*64){
                if (blockNO == 8+64) {
                    new_entry_table_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_table_block_id);
                    ParentInode->BlockPointer[9]=rd+new_entry_table_block_id*BLOCK_SIZE;
                }
                if ((blockNO-(8+64))%64 == 0) {// Need a new block for second indirect entry table
                    new_entry_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_block_id);
                    *(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)=rd+new_entry_block_id*BLOCK_SIZE;
                }
                *(*(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)+((blockNO-8-64)%64)*4) = (uint8_t)(new_block_id & 0x000000ff);
                *(*(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)+((blockNO-8-64)%64)*4+1) = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                *(*(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)+((blockNO-8-64)%64)*4+2) = (uint8_t)((new_block_id & 0x0000ff00)>>(2*BYTELEN));
                *(*(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)+((blockNO-8-64)%64)*4+3) = (uint8_t)((new_block_id & 0x0000ff00)>>(3*BYTELEN));
            }
            else{
#ifdef UL_DEBUG
                printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
            }
        }

        entry_pos=ParentInode->size%BLOCK_SIZE; // the position that the entry should be written to 
        NewDirEntry->filename = name;
        NewDirEntry->InodeNo = InodeNO;
        if (blockNO<=7 && blockNO>=0) {
            write_dir_entry(rd[ParentInode->BlockPointer[blockNO]*BLOCK_SIZE+entry_pos], NewDirEntry);
        }
        else if (blockNO>7 && blockNO<=7+64) {
            write_dir_entry(rd[*(ParentInode->BlockPointer[8]+(blockNO-8)*4)*BLOCK_SIZE+entry_pos], NewDirEntry);
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
            write_dir_entry(rd[*(*(ParentInode->BlockPointer[9])+(blockNO-8)*4)*BLOCK_SIZE+entry_pos], NewDirEntry);
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
            
        ParentInode->size += 16;
        update_inode(rd, ParentInodeNO, ParentInode);
        return 0;
    }    
    else
        return(-1); // -1 means creation fails
}

int create_dir (uint8_t* rd, int ParentDirInode, char* name)
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
    struct dir_entry* NewDirEntry;
    uint16_t InodeNO;
    int blockNO;
    int new_block_id;
    int new_entry_block_id;
    int new_entry_table_block_id;
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
	if(!(NewDirEntry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
    read_superblock(rd, SuperBlock);
    if (SuperBlock->FreeBlockNum != 0 && SuperBlock->FreeInodeNum != 0)
    {
        InodeNO = find_next_free_inode(rd);
        set_inode_bitmap(rd, InodeNO);
        Inode->type = (uint8_t) 0x0; // Initialize the type as directory file
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
                ParentInode->BlockPointer[blockNO] = new_block_id;
            else if (blockNO > 7 && blockNO <= 7+64){
                if (blockNO == 8) {
                    new_entry_block_id = find_next_free_block(rd); 
                    set_bitmap(rd, new_entry_block_id);
                    ParentInode->BlockPointer[8]=rd+new_entry_block_id*BLOCK_SIZE;
                }
                *(ParentInode->BlockPointer[8]+(blockNO-8)*4) = (uint8_t)(new_block_id & 0x000000ff);
                *(ParentInode->BlockPointer[8]+(blockNO-8)*4+1) = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                *(ParentInode->BlockPointer[8]+(blockNO-8)*4+2) = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                *(ParentInode->BlockPointer[8]+(blockNO-8)*4+3) = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
            }
            else if (blockNO > 7+64 && blockNO <= 7+64+64*64){
                if (blockNO == 8+64) {
                    new_entry_table_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_table_block_id);
                    ParentInode->BlockPointer[9]=rd+new_entry_table_block_id*BLOCK_SIZE;
                }
                if ((blockNO-(8+64))%64 == 0) {// Need a new block for second indirect entry table
                    new_entry_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_block_id);
                    *(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)=rd+new_entry_block_id*BLOCK_SIZE;
                }
                *(*(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)+((blockNO-8-64)%64)*4) = (uint8_t)(new_block_id & 0x000000ff);
                *(*(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)+((blockNO-8-64)%64)*4+1) = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                *(*(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)+((blockNO-8-64)%64)*4+2) = (uint8_t)((new_block_id & 0x0000ff00)>>(2*BYTELEN));
                *(*(ParentInode->BlockPointer[9]+(uint32_t)(blockNO-(8+64))*4/64)+((blockNO-8-64)%64)*4+3) = (uint8_t)((new_block_id & 0x0000ff00)>>(3*BYTELEN));
            }
            else{
#ifdef UL_DEBUG
                printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
            }
        }

        entry_pos=ParentInode->size%BLOCK_SIZE; // the position that the entry should be written to 
        NewDirEntry->filename = name;
        NewDirEntry->InodeNo = InodeNO;
        if (blockNO<=7 && blockNO>=0) {
            write_dir_entry(rd[ParentInode->BlockPointer[blockNO]*BLOCK_SIZE+entry_pos], NewDirEntry);
        }
        else if (blockNO>7 && blockNO<=7+64) {
            write_dir_entry(rd[*(ParentInode->BlockPointer[8]+(blockNO-8)*4)*BLOCK_SIZE+entry_pos], NewDirEntry);
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
            write_dir_entry(rd[*(*(ParentInode->BlockPointer[9])+(blockNO-8)*4)*BLOCK_SIZE+entry_pos], NewDirEntry);
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
            
        ParentInode->size += 16;
        update_inode(rd, ParentInodeNO, ParentInode);
        return 0;
    }    
    else
        return(-1); // -1 means creation fails
}

int remove_file (uint8_t* rd, int ParentInodeNO, int InodeNO, char* name)
{
    if (strlen(name) >= 14) {
#ifdef UL_DEBUG
        printf("File name is toooooo long!\n");
#endif
        return(-1);
    }
    struct inode* ParentInode;
    struct inode* Inode;
    struct dir_entry* ParentDirEntry;
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
        blockNO = (int)temp;
    else 
        blockNO = (int)temp-1;

    for (i=0;i<blockNO;i++)
    {
        if (i>=0 && i<=7)
            clr_bitmap(rd, Inode->BlockPointer[i]);
        else if (i>7 && i<=7+64)
            clr_bitmap(rd, *(Inode->BlockPointer[8]+(i-(8+64))*4));
        else if (i>7+64 && i<=7+64+64*64)
            clr_bitmap(rd, *(*(Inode->BlockPointer[9]+(i-(8+64)*4/64))+((i-(8+64))%64)*4));
    }

    //should strcmp with all the entries in ParentInode and delete the corresponding one.
}

int remove_dir (uint8_t* rd, int ParentInodeNO, int InodeNO, char* name)
{
    return(-1); // -1 means removal fails
}

//void switch_block (uint8_t* rd, int DirInodeNO, char)
