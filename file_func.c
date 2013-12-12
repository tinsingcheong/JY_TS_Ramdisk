#include "constant.h"
//#include "user_struct.h"
#include "ramdisk.h"
#include "file_func.h"

#define UL_DEBUG

#ifdef UL_DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

int write_file (uint8_t* rd, uint16_t InodeNO, int pos, char* string, int length)
{
    return(-1); // -1 means nothing is written into the file 
}

int read_file (uint8_t* rd, uint16_t InodeNO, int pos)
{
    return(-1); // -1 means nothing is read from the file
}

int read_dir (uint8_t* rd, uint16_t InodeNO)
{
    return(-1); // -1 means no such directory
}

int get_file_size (uint8_t* rd, uint16_t InodeNO)
{
    struct inode* Inode;
    read_inode(rd, InodeNO, Inode);
    return Inode->size; // return the size of the file 
}

int create_file (uint8_t* rd, uint16_t ParentInodeNO, char* name)
{
    if (strlen(name) >= 14) {
#ifdef UL_DEBUG
        printf("File name is toooooo long!\n");
#endif
        return(-1);
    }
    struct super_block* SuperBlock;
    struct inode* ParentInode;
    struct inode* Inode;
    struct dir_entry* NewDirEntry;
    uint16_t InodeNO;
    int blockNO;
    int new_block_id;
    int new_block_flag=0;
    int new_entry_block_id;
    int new_entry_table_block_id;
    uint8_t entry_pos;
    double temp;
	int k;
    
	if(!(SuperBlock=(struct super_block*)malloc(sizeof(struct super_block)))){
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
#ifdef UL_DEBUG
        printf("Acquired Inode is %u.\n", InodeNO);
#endif
        set_inode_bitmap(rd, InodeNO);
        Inode->type = (uint8_t) 0x01; // Initialize the type as regular file
        Inode->size = (uint32_t) 0x0; // Initialize the size as 0
        update_inode(rd, InodeNO, Inode); // Update the new Inode
        // Update the Parent Inode information
        read_inode(rd, ParentInodeNO, ParentInode);
#ifdef UL_DEBUG
        printf("Read: ParentInode size is %d.\n", ParentInode->size);
#endif
        temp = (double)(ParentInode->size)/BLOCK_SIZE;
        if (temp - (int)temp > 0) { // if the current blocks are not filled up
            blockNO = (int)temp;
            new_block_flag = 0;
        }
        else if (temp-(int)temp == 0) { // if the current blocks are filled up
            blockNO = (int)temp;
            new_block_id = find_next_free_block(rd);
            set_bitmap(rd, new_block_id);
            new_block_flag = 1;
        }
#ifdef UL_DEBUG
            printf("Temp is %lf.\n", temp);
            printf("Block # is %d.\n", blockNO);
#endif
        if (blockNO<=7 && blockNO>=0) {
            if (new_block_flag == 1) 
                ParentInode->BlockPointer[blockNO] = new_block_id;

            rd[ParentInode->BlockPointer[blockNO]*BLOCK_SIZE+ParentInode->size%BLOCK_SIZE] = new_block_id;
#ifdef UL_DEBUG
            printf("File is registered in the first 8 blocks\n");
#endif
        }
        else if (blockNO > 7 && blockNO <= 7+64){
            if (new_block_flag == 1) {
                if (blockNO == 8) { // Need to initiate the 9th block in the parent dir
                    new_entry_block_id = find_next_free_block(rd); 
                    set_bitmap(rd, new_entry_block_id);
                    ParentInode->BlockPointer[8]=new_entry_block_id;
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                } 
                else { // Need to assign a new block for the indirect list
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
            }

            rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4] = (uint8_t)(new_block_id & 0x000000ff);
            rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
            rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
            rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
#ifdef UL_DEBUG
            printf("File is registered in the 9th block\n");
#endif
        }
        else if (blockNO > 7+64 && blockNO <= 7+64+64*64){
            if (new_block_flag == 1) {
                if (blockNO == 8+64) { // Need to initiate the 10th block in the parent dir
                    new_entry_table_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_table_block_id);
                    ParentInode->BlockPointer[9]=new_entry_table_block_id;
                    new_entry_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_block_id);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE]=(uint8_t)(new_entry_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+1]=(uint8_t)((new_entry_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+2]=(uint8_t)((new_entry_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+3]=(uint8_t)((new_entry_block_id & 0xff000000)>>(3*BYTELEN));
                    rd[new_entry_block_id*BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[new_entry_block_id*BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[new_entry_block_id*BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[new_entry_block_id*BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
                else if ((blockNO-(8+64))%64 == 0) { // Need a new block for second indirect entry table
                    new_entry_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_block_id);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]=(uint8_t)(new_entry_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64+1]=(uint8_t)((new_entry_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64+2]=(uint8_t)((new_entry_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64+3]=(uint8_t)((new_entry_block_id & 0xff000000)>>(3*BYTELEN));
                    rd[new_entry_block_id*BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[new_entry_block_id*BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[new_entry_block_id*BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[new_entry_block_id*BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
                else {
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]*BLOCK_SIZE+((blockNO-8-64)%64)*4] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]*BLOCK_SIZE+((blockNO-8-64)%64)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]*BLOCK_SIZE+((blockNO-8-64)%64)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]*BLOCK_SIZE+((blockNO-8-64)%64)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
            }
#ifdef UL_DEBUG
            printf("File is registered in the 10th block\n");
#endif
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
        
        entry_pos=ParentInode->size%BLOCK_SIZE; // the position that the entry should be written to 
#ifdef UL_DEBUG
                printf("Entry position is %d.\n",entry_pos);
#endif
        strcpy(NewDirEntry->filename,name);
        NewDirEntry->InodeNo = InodeNO;
#ifdef UL_DEBUG
        printf("New entry name is: %s.\n", NewDirEntry->filename);
#endif
        if (blockNO<=7 && blockNO>=0) {
            write_dir_entry(&rd[ParentInode->BlockPointer[blockNO]*BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
                printf("Entry is registered in the first 8 blocks\n");
                for ( k = 0; k <=13; k++){
                    printf("%c", rd[ParentInode->BlockPointer[blockNO]*BLOCK_SIZE+entry_pos+k]);
                    (k==13)?(printf("\n")):(printf(""));
                }
#endif
        }
        else if (blockNO>7 && blockNO<=7+64) {
            write_dir_entry(&rd[rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4]*BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
                printf("Entry is registered in the 9th block\n");
#endif
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
            write_dir_entry(&rd[rd[rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(blockNO-(8+64))*4/64]+((blockNO-(8+64))%64)*4]*BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
                printf("Entry is registered in the 10th block\n");
#endif
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
            
        ParentInode->size += 16;
        update_inode(rd, ParentInodeNO, ParentInode);
#ifdef UL_DEBUG
        printf("Update: ParentInode size is%d.\n", ParentInode->size);
#endif
        return 0;
    }    
    else
        return(-1); // -1 means creation fails
}

int create_dir (uint8_t* rd, uint16_t ParentInodeNO, char* name)
{
    if (strlen(name) >= 14) {
#ifdef UL_DEBUG
        printf("File name is toooooo long!\n");
#endif
        return(-1);
    }
    struct super_block* SuperBlock;
    struct inode* ParentInode;
    struct inode* Inode;
    struct dir_entry* NewDirEntry;
    uint16_t InodeNO;
    int blockNO;
    int new_block_id;
    int new_block_flag=0;
    int new_entry_block_id;
    int new_entry_table_block_id;
    uint8_t entry_pos;
    double temp;
	int k;
    
	if(!(SuperBlock=(struct super_block*)malloc(sizeof(struct super_block)))){
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
#ifdef UL_DEBUG
        printf("Acquired Inode is %u.\n", InodeNO);
#endif
        set_inode_bitmap(rd, InodeNO);
        Inode->type = (uint8_t) 0x0; // Initialize the type as directory file
        Inode->size = (uint32_t) 0x0; // Initialize the size as 0
        update_inode(rd, InodeNO, Inode); // Update the new Inode
        // Update the Parent Inode information
        read_inode(rd, ParentInodeNO, ParentInode);
#ifdef UL_DEBUG
        printf("Read: ParentInode size is %d.\n", ParentInode->size);
#endif
        temp = (double)(ParentInode->size)/BLOCK_SIZE;
        if (temp - (int)temp > 0) { // if the current blocks are not filled up
            blockNO = (int)temp;
            new_block_flag = 0;
        }
        else if (temp-(int)temp == 0) { // if the current blocks are filled up
            blockNO = (int)temp+1;
            new_block_id = find_next_free_block(rd);
            set_bitmap(rd, new_block_id);
            new_block_flag = 1;
        }
#ifdef UL_DEBUG
            printf("Temp is %lf.\n", temp);
            printf("Block # is %d.\n", blockNO);
#endif
        if (blockNO<=7 && blockNO>=0) {
            if (new_block_flag == 1) 
                ParentInode->BlockPointer[blockNO] = new_block_id;

            rd[ParentInode->BlockPointer[blockNO]*BLOCK_SIZE+ParentInode->size%BLOCK_SIZE] = new_block_id;
#ifdef UL_DEBUG
            printf("File is registered in the first 8 blocks\n");
#endif
        }
        else if (blockNO > 7 && blockNO <= 7+64){
            if (new_block_flag == 1) {
                if (blockNO == 8) { // Need to initiate the 9th block in the parent dir
                    new_entry_block_id = find_next_free_block(rd); 
                    set_bitmap(rd, new_entry_block_id);
                    ParentInode->BlockPointer[8]=new_entry_block_id;
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                } 
                else { // Need to assign a new block for the indirect list
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
            }

            rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4] = (uint8_t)(new_block_id & 0x000000ff);
            rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
            rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
            rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
#ifdef UL_DEBUG
            printf("File is registered in the 9th block\n");
#endif
        }
        else if (blockNO > 7+64 && blockNO <= 7+64+64*64){
            if (new_block_flag == 1) {
                if (blockNO == 8+64) { // Need to initiate the 10th block in the parent dir
                    new_entry_table_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_table_block_id);
                    ParentInode->BlockPointer[9]=new_entry_table_block_id;
                    new_entry_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_block_id);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE]=(uint8_t)(new_entry_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+1]=(uint8_t)((new_entry_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+2]=(uint8_t)((new_entry_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+3]=(uint8_t)((new_entry_block_id & 0xff000000)>>(3*BYTELEN));
                    rd[new_entry_block_id*BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[new_entry_block_id*BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[new_entry_block_id*BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[new_entry_block_id*BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
                else if ((blockNO-(8+64))%64 == 0) { // Need a new block for second indirect entry table
                    new_entry_block_id = find_next_free_block(rd);
                    set_bitmap(rd, new_entry_block_id);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]=(uint8_t)(new_entry_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64+1]=(uint8_t)((new_entry_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64+2]=(uint8_t)((new_entry_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64+3]=(uint8_t)((new_entry_block_id & 0xff000000)>>(3*BYTELEN));
                    rd[new_entry_block_id*BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[new_entry_block_id*BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[new_entry_block_id*BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[new_entry_block_id*BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
                else {
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]*BLOCK_SIZE+((blockNO-8-64)%64)*4] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]*BLOCK_SIZE+((blockNO-8-64)%64)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]*BLOCK_SIZE+((blockNO-8-64)%64)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(uint32_t)(blockNO-(8+64))*4/64]*BLOCK_SIZE+((blockNO-8-64)%64)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
            }
#ifdef UL_DEBUG
            printf("File is registered in the 10th block\n");
#endif
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
        
        entry_pos=ParentInode->size%BLOCK_SIZE; // the position that the entry should be written to 
#ifdef UL_DEBUG
                printf("Entry position is %d.\n",entry_pos);
#endif
        strcpy(NewDirEntry->filename,name);
        NewDirEntry->InodeNo = InodeNO;
#ifdef UL_DEBUG
        printf("New entry name is: %s.\n", NewDirEntry->filename);
#endif
        if (blockNO<=7 && blockNO>=0) {
            write_dir_entry(&rd[ParentInode->BlockPointer[blockNO]*BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
                printf("Entry is registered in the first 8 blocks\n");
                for ( k = 0; k <=13; k++){
                    printf("%c", rd[ParentInode->BlockPointer[blockNO]*BLOCK_SIZE+entry_pos+k]);
                    (k==13)?(printf("\n")):(printf(""));
                }
#endif
        }
        else if (blockNO>7 && blockNO<=7+64) {
            write_dir_entry(&rd[rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(blockNO-8)*4]*BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
                printf("Entry is registered in the 9th block\n");
#endif
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
            write_dir_entry(&rd[rd[rd[ParentInode->BlockPointer[9]*BLOCK_SIZE+(blockNO-(8+64))*4/64]+((blockNO-(8+64))%64)*4]*BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
                printf("Entry is registered in the 10th block\n");
#endif
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
            
        ParentInode->size += 16;
        update_inode(rd, ParentInodeNO, ParentInode);
#ifdef UL_DEBUG
        printf("Update: ParentInode size is%d.\n", ParentInode->size);
#endif
        return 0;
    }    
    else
        return(-1); // -1 means creation fails
}

int remove_file (uint8_t* rd, uint16_t ParentInodeNO, uint16_t InodeNO, char* name)
{
    if (strlen(name) >= 14) {
#ifdef UL_DEBUG
        printf("File name is toooooo long!\n");
#endif
        return(-1);
    }
    struct super_block* SuperBlock;
    struct inode* ParentInode;
    struct inode* Inode;
    struct dir_entry* ParentDirEntry;
    int blockNO;
    int deleted_blockNO;
    uint8_t entry_pos;
    int i,j;
    int find_flag=0;

	if(!(SuperBlock=(struct super_block*)malloc(sizeof(struct super_block)))){
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
	if(!(ParentDirEntry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}

    read_inode(rd, ParentInodeNO, ParentInode);
    read_inode(rd, InodeNO, Inode);
    
    // Clear the inode bitmap and data bitmap of the selected file
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
            clr_bitmap(rd, rd[Inode->BlockPointer[8]*BLOCK_SIZE+(i-8)*4]);
        else if (i>7+64 && i<=7+64+64*64)
            clr_bitmap(rd, rd[rd[Inode->BlockPointer[9]*BLOCK_SIZE+(i-(8+64)*4/64)]*BLOCK_SIZE+((i-(8+64))%64)*4]);
    }
    
    // Find the entry that need to be deleted in the Parent directory
    temp = ParentInode->size/BLOCK_SIZE;
    if (temp-(int)temp>0)
        blockNO = (int)temp;
    else
        blockNO = (int)temp-1;

    for (i=0;i<blockNO;i++) {
        if (i>=0 && i<=7) {
            for (j=0;j<16;j++) {
                read_dir_entry(&rd[ParentInode->BlockPointer[i]*BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    find_flag = 1;
                    break;
                }
            }
        }
        else if (i>7 && i<=7+64) {
            for (j=0;j<16;j++) {
                read_dir_entry(&rd[rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(i-8)*4]*BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    find_flag = 1;
                    break;
                }
            }
        }
        else if (i>7+64 && i<=7+64+64*64) {
            for (j=0;j<16;j++) {
                read_dir_entry(&rd[rd[rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+((i-(8+64))*4/64)]*BLOCK_SIZE+((i-(8+64))%64)*4]+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    find_flag = 1;
                    break;
                }
            }
        }
        if (find_flag == 1) {
            deleted_blockNO=i;
            find_flag = 0;
            break;
        }
    }
    // Now ParentDirEntry should contain the entry that should be deleted
    int delete_flag=delete_dir_entry(rd, ParentInode, deleted_blockNO);
    if (delete_flag == -1)
        return(-1);
    update_inode(rd, ParentInodeNO, ParentInode);
    update_superblock(rd, SuperBlock);
    return 0;
}

int remove_dir (uint8_t* rd, uint16_t ParentInodeNO, uint16_t InodeNO, char* name)
{
    if (strlen(name) >= 14) {
#ifdef UL_DEBUG
        printf("File name is toooooo long!\n");
#endif
        return(-1);
    }
    struct super_block* SuperBlock;
    struct inode* ParentInode;
    struct inode* Inode;
    struct inode* DeleteInode;
    struct dir_entry* ParentDirEntry;
    struct dir_entry* DeleteEntry;
    int blockNO;
    int fileNO;
    int deleted_blockNO;
    uint8_t entry_pos;
    int i,j;
    int find_flag=0;
    
	if(!(SuperBlock=(struct super_block*)malloc(sizeof(struct super_block)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(Inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(DeleteInode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentInode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentDirEntry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(DeleteEntry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}

    read_inode(rd, ParentInodeNO, ParentInode);
    read_inode(rd, InodeNO, Inode);

    // Delete all the files inside the folder
    fileNO = Inode->size/ENTRY_SIZE;
    int blockNO_file;
    double temp;
    for (i=0;i<fileNO;i++) {
        temp = (i+1)/16;
        if (temp - (int)temp >0) 
            blockNO_file = (int)temp;
        else
            blockNO_file = (int)temp-1;
        if (blockNO_file>=0 && blockNO_file<=7) 
            read_dir_entry(&rd[Inode->BlockPointer[blockNO_file]*BLOCK_SIZE+i*ENTRY_SIZE], DeleteEntry);
        else if (blockNO_file>7 && blockNO_file<=7+64)
            read_dir_entry(&rd[rd[Inode->BlockPointer[8]*BLOCK_SIZE+(blockNO_file-8)*4]*BLOCK_SIZE+((i-8*16)%16)*ENTRY_SIZE], DeleteEntry);
        else if (blockNO_file>7+64 && blockNO_file<=7+64+64*64)
            read_dir_entry(&rd[rd[rd[Inode->BlockPointer[8]*BLOCK_SIZE+(blockNO_file-(8+64))*4/64]*BLOCK_SIZE+((blockNO_file-(8+64)%64)*4)]+((i-(8+64)*16)%16)*ENTRY_SIZE], DeleteEntry);
        else return(-1);
        
        read_inode(rd, DeleteEntry->InodeNo, DeleteInode);
        if (DeleteInode->type == 1)
            remove_file(rd, InodeNO, DeleteEntry->InodeNo, DeleteEntry->filename);
        else 
            remove_dir(rd, InodeNO, DeleteEntry->InodeNo, DeleteEntry->filename);
    }
    // Clear the inode bitmap and data bitmap of the selected file
    clr_inode_bitmap(rd, InodeNO);
    temp = Inode->size/BLOCK_SIZE;
    if (temp - (int)temp > 0)
        blockNO = (int)temp;
    else 
        blockNO = (int)temp-1;
    for (i=0;i<blockNO;i++)
    {
        if (i>=0 && i<=7)
            clr_bitmap(rd, Inode->BlockPointer[i]);
        else if (i>7 && i<=7+64)
            clr_bitmap(rd,rd[Inode->BlockPointer[8]*BLOCK_SIZE+(i-8)*4]);
        else if (i>7+64 && i<=7+64+64*64)
            clr_bitmap(rd, rd[rd[Inode->BlockPointer[9]*BLOCK_SIZE+(i-(8+64)*4/64)]*BLOCK_SIZE+((i-(8+64))%64)*4]);
    }
    
    // Find the entry that need to be deleted in the Parent directory
    temp = ParentInode->size/BLOCK_SIZE;
    if (temp-(int)temp>0)
        blockNO = (int)temp;
    else
        blockNO = (int)temp-1;

    for (i=0;i<blockNO;i++) {
        if (i>=0 && i<=7) {
            for (j=0;j<16;j++) {
                read_dir_entry(&rd[ParentInode->BlockPointer[i]*BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    find_flag = 1;
                    break;
                }
            }
        }
        else if (i>7 && i<=7+64) {
            for (j=0;j<16;j++) {
                read_dir_entry(&rd[rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+(i-8)*4]*BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    find_flag = 1;
                    break;
                }
            }
        }
        else if (i>7+64 && i<=7+64+64*64) {
            for (j=0;j<16;j++) {
                read_dir_entry(&rd[rd[rd[ParentInode->BlockPointer[8]*BLOCK_SIZE+((i-(8+64))*4/64)]*BLOCK_SIZE+((i-(8+64))%64)*4]+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    find_flag = 1;
                    break;
                }
            }
        }
        if (find_flag == 1) {
            deleted_blockNO=i;
            find_flag = 0;
            break;
        }
    }
    // Now ParentDirEntry should contain the entry that should be deleted
    int delete_flag=delete_dir_entry(rd, ParentInode, deleted_blockNO);
    if (delete_flag == -1)
        return(-1);
    update_inode(rd, ParentInodeNO, ParentInode);
    update_superblock(rd, SuperBlock);
    return 0; // -1 means removal fails
}

//void switch_block (uint8_t* rd, int DirInodeNO, char)
int delete_dir_entry(uint8_t* rd, struct inode* Inode, int deleted_blockNO)
{
    int blockNO;
    double temp = Inode->size/BLOCK_SIZE;
    if (temp-(int)temp > 0)
        blockNO = (int)temp;
    else blockNO = (int)temp-1;

    if (deleted_blockNO>=0 && deleted_blockNO<=7) {
        if (blockNO>=0 && deleted_blockNO<=7) {
            Inode->BlockPointer[deleted_blockNO] = Inode->BlockPointer[blockNO];
            Inode->size -= 16;
        }    
        else if (blockNO>7 && blockNO<=7+64) {
            Inode->BlockPointer[deleted_blockNO] = (uint32_t)rd[Inode->BlockPointer[8]*256+(blockNO-8)*4];
            Inode->size -= 16;
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
            Inode->BlockPointer[deleted_blockNO] = (uint32_t)rd[rd[Inode->BlockPointer[8]*256+(blockNO-(8+64))*4/64]+((blockNO-(8+64))%64)*4];
            Inode->size -= 16;
        }
        else return(-1);
    }
    else if (deleted_blockNO>7 && deleted_blockNO<=7+64) {
        if (blockNO>=0 && deleted_blockNO<=7) {
            rd[Inode->BlockPointer[deleted_blockNO]*256+(deleted_blockNO-8)*4] = Inode->BlockPointer[blockNO];
            Inode->size -= 16;
        }    
        else if (blockNO>7 && blockNO<=7+64) {
            rd[Inode->BlockPointer[deleted_blockNO]*256+(deleted_blockNO-8)*4] = (uint32_t)rd[Inode->BlockPointer[8]*256+(blockNO-8)*4];
            Inode->size -= 16;
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
            rd[Inode->BlockPointer[deleted_blockNO]*256+(deleted_blockNO-8)*4] = (uint32_t)rd[rd[Inode->BlockPointer[8]*256+(blockNO-(8+64))*4/64]+((blockNO-(8+64))%64)*4];
            Inode->size -= 16;
        }
        else return(-1);
    }
    else if (deleted_blockNO>7+64 && deleted_blockNO<=7+64+64*64){
        if (blockNO>=0 && deleted_blockNO<=7) {
            rd[rd[Inode->BlockPointer[deleted_blockNO]*256+(deleted_blockNO-(8+64))*4/64]*256+((blockNO-(8+64))%64)*4] = Inode->BlockPointer[blockNO];
            Inode->size -= 16;
        }    
        else if (blockNO>7 && blockNO<=7+64) {
            rd[rd[Inode->BlockPointer[deleted_blockNO]*256+(deleted_blockNO-(8+64))*4/64]*256+((blockNO-(8+64))%64)*4] = (uint32_t)rd[Inode->BlockPointer[8]*256+(blockNO-8)*4];
            Inode->size -= 16;
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
            rd[rd[Inode->BlockPointer[deleted_blockNO]*256+(deleted_blockNO-(8+64))*4/64]*256+((blockNO-(8+64))%64)*4] = (uint32_t)rd[rd[Inode->BlockPointer[8]*256+(blockNO-(8+64))*4/64]+((blockNO-(8+64))%64)*4];
            Inode->size -= 16;
        }
        else return(-1);
    }
    else return(-1);
    return 0;
}
