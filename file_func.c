#include "constant.h"
//#include "user_struct.h"
#include "ramdisk.h"
#include "file_func.h"

//#define UL_DEBUG

//#define KL_DEBUG

#ifndef UL_DEBUG
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h> /* error codes */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#endif

#ifdef UL_DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

int get_file_size (uint8_t* rd, uint16_t InodeNO)
{
    struct rd_inode* Inode;
	int tmp;
#ifdef UL_DEBUG
	if(!(Inode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
		printf(" NO mem\n");
		exit (-1);
	}
#endif
#ifndef UL_DEBUG
	if(!(Inode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> NO mem\n");
		return -1;
	}
#endif
    read_inode(rd, InodeNO, Inode);
	if(Inode->type==1){
		tmp=Inode->size;
#ifndef UL_DEBUG
		vfree(Inode);
#endif
		return tmp; // return the size of the file 
	}
	else{
#ifndef UL_DEBUG
		vfree(Inode);
#endif
		return -1;//dir file should return -1
	}
}

int create_file (uint8_t* rd, uint16_t ParentInodeNO, char* name, int mode)
{
    if (strlen(name) >= 14) {
#ifdef UL_DEBUG
        printf("File name is toooooo long!\n");
#endif
#ifndef UL_DEBUG
		printk("<1> File name is toooooo long!\n");
#endif

        return(-1);
    }
    struct rd_super_block* SuperBlock;
    struct rd_inode* ParentInode;
    struct rd_inode* Inode;
    struct dir_entry* NewDirEntry;
    uint16_t InodeNO;
    int blockNO;
    int new_block_id;
    int new_block_flag=0;
    int new_entry_block_id;
    int new_entry_table_block_id;
    uint8_t entry_pos;
    int temp;
	int k;
#ifdef UL_DEBUG   
	if(!(SuperBlock=(struct rd_super_block*)malloc(sizeof(struct rd_super_block)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(Inode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentInode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(NewDirEntry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
#endif
#ifndef UL_DEBUG
	if(!(SuperBlock=(struct rd_super_block*)vmalloc(sizeof(struct rd_super_block)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(Inode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(ParentInode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(NewDirEntry=(struct dir_entry*)vmalloc(sizeof(struct dir_entry)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
#endif

    read_superblock(rd, SuperBlock);
    if (SuperBlock->FreeBlockNum != 0 && SuperBlock->FreeInodeNum != 0)
    {
        InodeNO = find_next_free_inode(rd);
        if (InodeNO == 0xFFFF) {
#ifdef UL_DEBUG
            printf("There is no free inode anymore.\n");
#endif
#ifndef UL_DEBUG
            printk("<1> There is no free inode anymore.\n");
#endif

            return(-1);
        }
#ifdef UL_DEBUG
//        printf("Acquired Inode is %u.\n", InodeNO);
#endif
        set_inode_bitmap(rd, InodeNO);
        Inode->type = (uint8_t) 0x01; // Initialize the type as regular file
        Inode->size = (uint32_t) 0x0; // Initialize the size as 0
		Inode->mode = mode;
		printk("Create_File: The mode is %d.\n", Inode->mode);
        update_inode(rd, InodeNO, Inode); // Update the new Inode
        // Update the Parent Inode information
        read_inode(rd, ParentInodeNO, ParentInode);

        if (find_same_name(rd, ParentInode,name) == -1){
#ifdef UL_DEBUG
			printf("Find the same name!\n");
#endif
            return(-1);
		}
#ifdef UL_DEBUG
//        printf("Read: ParentInode size is %d.\n", ParentInode->size);
#endif
        temp = (ParentInode->size)%RD_BLOCK_SIZE;
        if (temp > 0) { // if the current blocks are not filled up
            blockNO = (ParentInode->size)/RD_BLOCK_SIZE;
            new_block_flag = 0;
        }
        else if (temp == 0) { // if the current blocks are filled up
            blockNO = (ParentInode->size)/RD_BLOCK_SIZE;
            new_block_id = find_next_free_block(rd);
            if (new_block_id == -1) {
#ifdef UL_DEBUG
                printf("There is no free block anymore.\n");
#endif
#ifndef UL_DEBUG
                printk("<1> There is no free block anymore.\n");
#endif

                return(-1);
            }
            set_bitmap(rd, new_block_id);
            new_block_flag = 1;
        }
#ifdef UL_DEBUG
//            printf("Temp is %lf.\n", temp);
//            printf("Block # is %d.\n", blockNO);
#endif
        if (blockNO<=7 && blockNO>=0) {
            if (new_block_flag == 1) 
                ParentInode->BlockPointer[blockNO] = new_block_id;

            rd[ParentInode->BlockPointer[blockNO]*RD_BLOCK_SIZE+ParentInode->size%RD_BLOCK_SIZE] = new_block_id;
#ifdef UL_DEBUG
//            printf("File is registered in the first 8 blocks\n");
#endif
        }
        else if (blockNO > 7 && blockNO <= 7+64){
            if (new_block_flag == 1) {
                if (blockNO == 8) { // Need to initiate the 9th block in the parent dir
                    new_entry_block_id = find_next_free_block(rd); 
                    if (new_entry_block_id == -1) {
#ifdef UL_DEBUG
                        printf("There is no free block anymore.\n");
#endif
#ifndef UL_DEBUG
                        printk("<1> There is no free block anymore.\n");
#endif

                        return(-1);
                    }
                    set_bitmap(rd, new_entry_block_id);
                    ParentInode->BlockPointer[8]=new_entry_block_id;
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                } 
                else { // Need to assign a new block for the indirect list
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
            }

  //          rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4] = (uint8_t)(new_block_id & 0x000000ff);
    //        rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
      //      rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
        //    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
#ifdef UL_DEBUG
//            printf("File is registered in the 9th block\n");
#endif
        }
        else if (blockNO > 7+64 && blockNO <= 7+64+64*64){
            if (new_block_flag == 1) {
                if (blockNO == 8+64) { // Need to initiate the 10th block in the parent dir
                    new_entry_table_block_id = find_next_free_block(rd);
                    if (new_entry_table_block_id == -1) {
#ifdef UL_DEBUG
                        printf("There is no free block anymore.\n");
#endif
#ifndef UL_DEBUG
                        printk("<1> There is no free block anymore.\n");
#endif

                        return(-1);
                    }

                    set_bitmap(rd, new_entry_table_block_id);
                    ParentInode->BlockPointer[9]=new_entry_table_block_id;
                    new_entry_block_id = find_next_free_block(rd);
                    if (new_entry_block_id == -1) {
#ifdef UL_DEBUG
                        printf("There is no free block anymore.\n");
#endif
#ifndef UL_DEBUG
                        printk("<1> There is no free block anymore.\n");
#endif

                        return(-1);
                    }
                    set_bitmap(rd, new_entry_block_id);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE]=(uint8_t)(new_entry_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+1]=(uint8_t)((new_entry_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+2]=(uint8_t)((new_entry_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+3]=(uint8_t)((new_entry_block_id & 0xff000000)>>(3*BYTELEN));
                    rd[new_entry_block_id*RD_BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[new_entry_block_id*RD_BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[new_entry_block_id*RD_BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[new_entry_block_id*RD_BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
                else if ((blockNO-(8+64))%64 == 0) { // Need a new block for second indirect entry table
                    new_entry_block_id = find_next_free_block(rd);
                    if (new_entry_block_id == -1) {
#ifdef UL_DEBUG
                        printf("There is no free block anymore.\n");
#endif
#ifndef UL_DEBUG
                        printk("<1> There is no free block anymore.\n");
#endif

                        return(-1);
                    }
                    set_bitmap(rd, new_entry_block_id);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]=(uint8_t)(new_entry_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4+1]=(uint8_t)((new_entry_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4+2]=(uint8_t)((new_entry_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4+3]=(uint8_t)((new_entry_block_id & 0xff000000)>>(3*BYTELEN));
                    rd[new_entry_block_id*RD_BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[new_entry_block_id*RD_BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[new_entry_block_id*RD_BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[new_entry_block_id*RD_BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
                else {
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]*RD_BLOCK_SIZE+((blockNO-8-64)%64)*4] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]*RD_BLOCK_SIZE+((blockNO-8-64)%64)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]*RD_BLOCK_SIZE+((blockNO-8-64)%64)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]*RD_BLOCK_SIZE+((blockNO-8-64)%64)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
            }
#ifdef UL_DEBUG
//            printf("File is registered in the 10th block\n");
#endif
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
        
        entry_pos=ParentInode->size%RD_BLOCK_SIZE; // the position that the entry should be written to 
#ifdef UL_DEBUG
//                printf("Entry position is %d.\n",entry_pos);
#endif
        strcpy(NewDirEntry->filename,name);
        NewDirEntry->InodeNo = InodeNO;
#ifdef UL_DEBUG
//        printf("New entry name is: %s.\n", NewDirEntry->filename);
#endif
        if (blockNO<=7 && blockNO>=0) {
            write_dir_entry(&rd[ParentInode->BlockPointer[blockNO]*RD_BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
//                printf("Entry is registered in the first 8 blocks\n");
                for ( k = 0; k <=13; k++){
                    printf("%c", rd[ParentInode->BlockPointer[blockNO]*RD_BLOCK_SIZE+entry_pos+k]);
                    (k==13)?(printf("\n")):(printf(""));
                }
#endif
        }
        else if (blockNO>7 && blockNO<=7+64) {
         //   write_dir_entry(&rd[rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4]*RD_BLOCK_SIZE+entry_pos], NewDirEntry);
			write_dir_entry(&rd[(*((uint32_t*)(rd+ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4)))*RD_BLOCK_SIZE+entry_pos], NewDirEntry);

#ifdef UL_DEBUG
//                printf("Entry is registered in the 9th block\n");
//				printf("The block is %d\n",(*((uint32_t*)(rd+ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4))));
                for ( k = 0; k <=13; k++){
                    printf("%c", rd[(*((uint32_t*)(rd+ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4)))*RD_BLOCK_SIZE+entry_pos+k]);
                    (k==13)?(printf("\n")):(printf(""));
                }
#endif
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
	//This situation won't happen because the max number of file is 1024, the inodes 9th block (the single indirect block is able to cover 1024 files
/*
            write_dir_entry(&rd[rd[rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(blockNO-(8+64))*4/64]+((blockNO-(8+64))%64)*4]*RD_BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
                printf("Entry is registered in the 10th block\n");
#endif*/
        }
        else{
#ifdef UL_DEBUG
            printf("Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
            
        ParentInode->size += 16;
        update_inode(rd, ParentInodeNO, ParentInode);
#ifdef UL_DEBUG
        printf("Update: ParentInode size is %d.\n", ParentInode->size);
#endif
		partial_update_superblock(rd);

#ifndef UL_DEBUG
		vfree(SuperBlock);
		vfree(Inode);
		vfree(ParentInode);
		vfree(NewDirEntry);
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
#ifndef UL_DEBUG
        printk("<1> File name is toooooo long!\n");
#endif

        return(-1);
    }
    struct rd_super_block* SuperBlock;
    struct rd_inode* ParentInode;
    struct rd_inode* Inode;
    struct dir_entry* NewDirEntry;
    uint16_t InodeNO;
    int blockNO;
    int new_block_id;
    int new_block_flag=0;
    int new_entry_block_id;
    int new_entry_table_block_id;
    uint8_t entry_pos;
    int temp;
	int k;
#ifdef UL_DEBUG  
	if(!(SuperBlock=(struct rd_super_block*)malloc(sizeof(struct rd_super_block)))){

		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(Inode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentInode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(NewDirEntry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
#endif
#ifndef UL_DEBUG
	if(!(SuperBlock=(struct rd_super_block*)vmalloc(sizeof(struct rd_super_block)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(Inode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(ParentInode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(NewDirEntry=(struct dir_entry*)vmalloc(sizeof(struct dir_entry)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
#endif


	read_superblock(rd, SuperBlock);
#ifdef UL_DEBUG
  //  printf("FREE inodes %d Free blocks %d\n", SuperBlock->FreeInodeNum, SuperBlock->FreeBlockNum);
#endif

    if (SuperBlock->FreeBlockNum != 0 && SuperBlock->FreeInodeNum != 0)
    {
        InodeNO = find_next_free_inode(rd);
        if (InodeNO == 0xFFFF) {
#ifdef UL_DEBUG
//            printf("create_dir: There is no free block anymore.\n");
#endif
            return(-1);
        }
#ifdef UL_DEBUG
 //       printf("create_dir: Acquired Inode is %u.\n", InodeNO);
#endif
        set_inode_bitmap(rd, InodeNO);
        Inode->type = (uint8_t) 0x0; // Initialize the type as regular file
        Inode->size = (uint32_t) 0x0; // Initialize the size as 0
		Inode->mode = 0;
        update_inode(rd, InodeNO, Inode); // Update the new Inode
        // Update the Parent Inode information
        read_inode(rd, ParentInodeNO, ParentInode);
#ifdef UL_DEBUG
    //    printf("create_dir0: Read: ParentInode size is %d.\n", ParentInode->size);
#endif
        if (find_same_name(rd, ParentInode, name) == -1){
#ifdef UL_DEBUG
			printf("Find the same name!\n");
#endif
            return(-1);
		}
#ifdef UL_DEBUG
    //    printf("create_dir1: Read: ParentInode size is %d.\n", ParentInode->size);
#endif

        temp = (ParentInode->size)%RD_BLOCK_SIZE;
        if (temp > 0) { // if the current blocks are not filled up
            blockNO = (ParentInode->size)/RD_BLOCK_SIZE;
            new_block_flag = 0;
        }
        else if (temp == 0) { // if the current blocks are filled up
            blockNO = (ParentInode->size)/RD_BLOCK_SIZE;
            new_block_id = find_next_free_block(rd);
            if (new_block_id == -1) {
#ifdef UL_DEBUG
//                printf("create_dir: There is no free block anymore.\n");
#endif
                return(-1);
            }
            set_bitmap(rd, new_block_id);
            new_block_flag = 1;
        }
#ifdef UL_DEBUG
//            printf("create_dir: Temp is %lf.\n", temp);
//            printf("create_dir: Block # is %d.\n", blockNO);
#endif
        if (blockNO<=7 && blockNO>=0) {
            if (new_block_flag == 1) 
                ParentInode->BlockPointer[blockNO] = new_block_id;

            rd[ParentInode->BlockPointer[blockNO]*RD_BLOCK_SIZE+ParentInode->size%RD_BLOCK_SIZE] = new_block_id;
#ifdef UL_DEBUG
 //           printf("create_dir: File is registered in the first 8 blocks\n");
#endif
        }
        else if (blockNO > 7 && blockNO <= 7+64){
            if (new_block_flag == 1) {
                if (blockNO == 8) { // Need to initiate the 9th block in the parent dir
#ifdef UL_DEBUG
//            printf("create_dir: I'm here!\n");
#endif
                    new_entry_block_id = find_next_free_block(rd); 
                    if (new_entry_block_id == -1) {
#ifdef UL_DEBUG
//                        printf("create_dir: There is no free block anymore.\n");
#endif
                        return(-1);
                    }
                    set_bitmap(rd, new_entry_block_id);
                    ParentInode->BlockPointer[8]=new_entry_block_id;
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                } 
                else { // Need to assign a new block for the indirect list
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
            }

            rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4] = (uint8_t)(new_block_id & 0x000000ff);
            rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
            rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
            rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
#ifdef UL_DEBUG
//            printf("create_dir: File is registered in the 9th block\n");
#endif
        }
        else if (blockNO > 7+64 && blockNO <= 7+64+64*64){
            if (new_block_flag == 1) {
                if (blockNO == 8+64) { // Need to initiate the 10th block in the parent dir
                    new_entry_table_block_id = find_next_free_block(rd);
                    if (new_entry_table_block_id == -1) {
#ifdef UL_DEBUG
//                        printf("create_dir: There is no free block anymore.\n");
#endif
                        return(-1);
                    }
                    set_bitmap(rd, new_entry_table_block_id);
                    ParentInode->BlockPointer[9]=new_entry_table_block_id;
                    new_entry_block_id = find_next_free_block(rd);
                    if (new_entry_block_id == -1) {
#ifdef UL_DEBUG
//                        printf("create_dir: There is no free block anymore.\n");
#endif
                        return(-1);
                    }
                    set_bitmap(rd, new_entry_block_id);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE]=(uint8_t)(new_entry_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+1]=(uint8_t)((new_entry_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+2]=(uint8_t)((new_entry_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+3]=(uint8_t)((new_entry_block_id & 0xff000000)>>(3*BYTELEN));
                    rd[new_entry_block_id*RD_BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[new_entry_block_id*RD_BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[new_entry_block_id*RD_BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[new_entry_block_id*RD_BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
                else if ((blockNO-(8+64))%64 == 0) { // Need a new block for second indirect entry table
                    new_entry_block_id = find_next_free_block(rd);
                    if (new_entry_block_id == -1) {
#ifdef UL_DEBUG
//                        printf("create_dir: There is no free block anymore.\n");
#endif
                        return(-1);
                    }
                    set_bitmap(rd, new_entry_block_id);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]=(uint8_t)(new_entry_block_id & 0x000000ff);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4+1]=(uint8_t)((new_entry_block_id & 0x0000ff00)>>BYTELEN);
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4+2]=(uint8_t)((new_entry_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4+3]=(uint8_t)((new_entry_block_id & 0xff000000)>>(3*BYTELEN));
                    rd[new_entry_block_id*RD_BLOCK_SIZE] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[new_entry_block_id*RD_BLOCK_SIZE+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[new_entry_block_id*RD_BLOCK_SIZE+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[new_entry_block_id*RD_BLOCK_SIZE+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
                else {
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]*RD_BLOCK_SIZE+((blockNO-8-64)%64)*4] = (uint8_t)(new_block_id & 0x000000ff);
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]*RD_BLOCK_SIZE+((blockNO-8-64)%64)*4+1] = (uint8_t)((new_block_id & 0x0000ff00)>>BYTELEN);
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]*RD_BLOCK_SIZE+((blockNO-8-64)%64)*4+2] = (uint8_t)((new_block_id & 0x00ff0000)>>(2*BYTELEN));
                    rd[(uint32_t)rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(uint32_t)((blockNO-(8+64))/64)*4]*RD_BLOCK_SIZE+((blockNO-8-64)%64)*4+3] = (uint8_t)((new_block_id & 0xff000000)>>(3*BYTELEN));
                }
            }
#ifdef UL_DEBUG
//            printf("create_dir: File is registered in the 10th block\n");
#endif
        }
        else{
#ifdef UL_DEBUG
//            printf("create_dir: Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
        
        entry_pos=ParentInode->size%RD_BLOCK_SIZE; // the position that the entry should be written to 
#ifdef UL_DEBUG
        //        printf("create_dir2: ParentInode size is %d Entry position is %d.\n",ParentInode->size, entry_pos);
#endif

        strcpy(NewDirEntry->filename,name);
        NewDirEntry->InodeNo = InodeNO;
#ifdef UL_DEBUG
//        printf("create_dir: New entry name is: %s.\n", NewDirEntry->filename);
#endif
        if (blockNO<=7 && blockNO>=0) {
            write_dir_entry(&rd[ParentInode->BlockPointer[blockNO]*RD_BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
//                printf("create_dir: Entry is registered in the first 8 blocks\n");
//                for ( k = 0; k <=13; k++){
//                    printf("%c", rd[ParentInode->BlockPointer[blockNO]*RD_BLOCK_SIZE+entry_pos+k]);
//                    (k==13)?(printf("\n")):(printf(""));
//                }
#endif
        }
        else if (blockNO>7 && blockNO<=7+64) {
         //   write_dir_entry(&rd[rd[ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4]*RD_BLOCK_SIZE+entry_pos], NewDirEntry);
			write_dir_entry(&rd[(*((uint32_t*)(rd+ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4)))*RD_BLOCK_SIZE+entry_pos], NewDirEntry);

#ifdef UL_DEBUG
//                printf("create_dir: Entry is registered in the 9th block\n");
//				printf("create_dir: The block is %d\n",(*((uint32_t*)(rd+ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4))));
//                for ( k = 0; k <=13; k++){
//                    printf("%c", rd[(*((uint32_t*)(rd+ParentInode->BlockPointer[8]*RD_BLOCK_SIZE+(blockNO-8)*4)))*RD_BLOCK_SIZE+entry_pos+k]);
//                    (k==13)?(printf("\n")):(printf(""));
//                }
#endif
        }
        else if (blockNO>7+64 && blockNO<=7+64+64*64) {
	//This situation won't happen because the max number of file is 1024, the inodes 9th block (the single indirect block is able to cover 1024 files
/*
            write_dir_entry(&rd[rd[rd[ParentInode->BlockPointer[9]*RD_BLOCK_SIZE+(blockNO-(8+64))*4/64]+((blockNO-(8+64))%64)*4]*RD_BLOCK_SIZE+entry_pos], NewDirEntry);
#ifdef UL_DEBUG
                printf("Entry is registered in the 10th block\n");
#endif*/
        }
        else{
#ifdef UL_DEBUG
//            printf("create_dir: Block # is smaller than 0 or bigger than the limit!\n");
#endif
        }
            
        ParentInode->size += 16;
        update_inode(rd, ParentInodeNO, ParentInode);
#ifdef UL_DEBUG
     //   printf("create_dir3: Update: ParentInode size %d is %d.\n", ParentInodeNO, ParentInode->size);
#endif


		partial_update_superblock(rd);
#ifndef UL_DEBUG
		vfree(SuperBlock);
		vfree(Inode);
		vfree(ParentInode);
		vfree(NewDirEntry);
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
        printf("remove_file: File name is toooooo long!\n");
#endif
#ifndef UL_DEBUG
        printk("<1> remove_file: File name is toooooo long!\n");
#endif

        return(-1);
    }
    struct rd_super_block* SuperBlock;
    struct rd_inode* ParentInode;
    struct rd_inode* Inode;
    struct dir_entry* ParentDirEntry;
    int blockNO;
    int entryNO;
    int deleted_blockNO;
    int deleted_entryNO;
    int read_blockNO;
    int read_block_tableNO;
    int read_double_tableNO;
    int i,j;
    uint8_t entry_pos;
    int temp;
#ifdef UL_DEBUG
	if(!(SuperBlock=(struct rd_super_block*)malloc(sizeof(struct rd_super_block)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(Inode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentInode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentDirEntry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
#endif
#ifndef UL_DEBUG
	if(!(SuperBlock=(struct rd_super_block*)vmalloc(sizeof(struct rd_super_block)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(Inode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(ParentInode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(ParentDirEntry=(struct dir_entry*)vmalloc(sizeof(struct dir_entry)))){
		printk("<1> No mem space!\n");
		return (-1);
	}

#endif

#ifdef UL_DEBUG
//    read_superblock(rd, SuperBlock);
//    printf("remove_file: Free blocks: %d; Free inodes: %d.\n", SuperBlock->FreeBlockNum, SuperBlock->FreeInodeNum);
#endif

    read_inode(rd, ParentInodeNO, ParentInode);
    read_inode(rd, InodeNO, Inode);
    
    // Clear the inode bitmap and data bitmap of the selected file
    clr_inode_bitmap(rd, InodeNO);
    temp = (Inode->size)%RD_BLOCK_SIZE;
    if ((temp > 0)||(Inode->size == 0))
        blockNO = (Inode->size)/RD_BLOCK_SIZE;
    else 
        blockNO = (Inode->size)/RD_BLOCK_SIZE-1;

    // Clear the file blocks in the bitmap 
    for (i=0;i<blockNO;i++)
    {
        if (i>=0 && i<=7) {
            read_blockNO = Inode->BlockPointer[i];
#ifndef UL_DEBUG
			printk("[remove_file] i=%d The block is going to be cleared is %d\n",i,read_blockNO);
#endif

            clr_bitmap(rd, read_blockNO);


        }
        else if (i>7 && i<=7+64) {
            read_block_tableNO = Inode->BlockPointer[8];
            read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+(i-8)*4));
#ifndef UL_DEBUG
			printk("[remove_file] i=%d The block is going to be cleared is %d and %d\n",i,read_blockNO,read_block_tableNO);
#endif

            clr_bitmap(rd, read_blockNO);
            clr_bitmap(rd, read_block_tableNO);
        }
        else if (i>7+64 && i<=7+64+64*64) {
            read_double_tableNO = Inode->BlockPointer[9];
            read_block_tableNO = *((uint32_t*)(rd+read_double_tableNO*RD_BLOCK_SIZE+((i-(8+64))/64)*4));
            read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+((i-(8+64))%64)*4));
#ifndef UL_DEBUG
			printk("[remove_file] i=%d The block is going to be cleared is %d and %d and %d\n",i,read_blockNO,read_block_tableNO,read_double_tableNO);
#endif

            clr_bitmap(rd, read_blockNO);
            clr_bitmap(rd, read_block_tableNO);
            clr_bitmap(rd, read_double_tableNO);
        }
    }
   
#ifndef UL_DEBUG
	printk("<1> file content has been cleared and next step is to modify block in parent inode\n");
#endif
    // Find the entry that need to be deleted in the Parent directory
    blockNO = (ParentInode->size%RD_BLOCK_SIZE)?(ParentInode->size/RD_BLOCK_SIZE+1):ParentInode->size/RD_BLOCK_SIZE;
#ifdef UL_DEBUG
    entryNO = ParentInode->size/ENTRY_SIZE;
//    printf("remove_file: The total entry NO is %d.\n", entryNO);
#endif
    for (i=0;i<blockNO;i++) {
        if (i>=0 && i<=7) {
            for (j=0;j<16;j++) {
                read_blockNO = ParentInode->BlockPointer[i];
                read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
#ifdef UL_DEBUG
//                    printf("remove_file: The entry name is %s.\n", ParentDirEntry->filename);
#endif
                if (strcmp(ParentDirEntry->filename, name)==0){
                    deleted_blockNO=i;
                    deleted_entryNO=i*16+j+1;
#ifdef UL_DEBUG
//                    printf("remove_file: The entry need to be deleted is %d.\nFrom the first 8 blocks.\n", deleted_entryNO);
#endif
                    break;
                }
            }
        }
        else if (i>7 && i<=7+64) {
            for (j=0;j<16;j++) {
                read_block_tableNO = ParentInode->BlockPointer[8];
                read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+(i-8)*4));
                read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    deleted_blockNO=i;
                    deleted_entryNO=i*16+j+1;
                    break;
                }
            }
        }
        else if (i>7+64 && i<=7+64+64*64) {
            for (j=0;j<16;j++) {
                read_double_tableNO = ParentInode->BlockPointer[9];
                read_block_tableNO = *((uint32_t*)(rd+read_double_tableNO*RD_BLOCK_SIZE+((i-(8+64))/64)*4));
                read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+((i-(8+64))%64)*4));
                read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    deleted_blockNO=i;
                    deleted_entryNO=i*16+j+1;
                    break;
                }
            }
        }
    }
    // Now ParentDirEntry should contain the entry that should be deleted
#ifdef UL_DEBUG
//    printf("remove_file: The entry we want to delete is %d.\n", deleted_entryNO);
#endif
    int delete_flag=delete_dir_entry(rd, ParentInode, ParentInodeNO, deleted_entryNO);
    if (delete_flag == -1)
        return(-1);
    partial_update_superblock(rd);
#ifdef UL_DEBUG
//    printf("remove_file: The entry we deleted is %d.\n", deleted_entryNO);
//    read_superblock(rd, SuperBlock);
//    printf("remove_file: Free blocks: %d; Free inodes: %d.\n", SuperBlock->FreeBlockNum, SuperBlock->FreeInodeNum);
#endif
#ifndef UL_DEBUG
		vfree(SuperBlock);
		vfree(Inode);
		vfree(ParentInode);
		vfree(ParentDirEntry);
#endif

    return 0;
}

int remove_dir (uint8_t* rd, uint16_t ParentInodeNO, uint16_t InodeNO, char* name)
{
    if (strlen(name) >= 14) {
#ifdef UL_DEBUG
        printf("remove_dir: File name is toooooo long!\n");
#endif
#ifndef UL_DEBUG
        printk("<1> remove_dir: File name is toooooo long!\n");
#endif
        return(-1);
    }
    struct rd_super_block* SuperBlock;
    struct rd_inode* ParentInode;
    struct rd_inode* Inode;
    struct rd_inode* DeleteInode;
    struct dir_entry* ParentDirEntry;
    struct dir_entry* DeleteEntry;
    int read_blockNO;
    int read_block_tableNO;
    int read_double_tableNO;
    int fileNO;
    int blockNO;
    int blockNO_file;
    int entryNO;
    int deleted_blockNO;
    int deleted_entryNO;
    int temp;
    uint8_t entry_pos;
    int i,j;
#ifdef UL_DEBUG   
	if(!(SuperBlock=(struct rd_super_block*)malloc(sizeof(struct rd_super_block)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
    }
	if(!(Inode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(DeleteInode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
	if(!(ParentInode=(struct rd_inode*)malloc(sizeof(struct rd_inode)))){
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
#endif
#ifndef UL_DEBUG
	if(!(SuperBlock=(struct rd_super_block*)vmalloc(sizeof(struct rd_super_block)))){
		printk("<1> No mem space!\n");
		return (-1);
    }
	if(!(Inode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(DeleteInode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(ParentInode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(ParentDirEntry=(struct dir_entry*)vmalloc(sizeof(struct dir_entry)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
	if(!(DeleteEntry=(struct dir_entry*)vmalloc(sizeof(struct dir_entry)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
#endif



    read_inode(rd, ParentInodeNO, ParentInode);
    read_inode(rd, InodeNO, Inode);

    // Delete all the files inside the folder
    fileNO = Inode->size/ENTRY_SIZE;
#ifdef UL_DEBUG
//        printf("remove_dir: The inode size is %d.\n",Inode->size);
#endif
    for (i=fileNO-1;i>=0;i--) {
#ifdef UL_DEBUG
//        printf("remove_dir: Need to delete the children files.\n");
#endif
        temp = i%16;
#ifdef UL_DEBUG
//        printf("remove_dir: Temp is %lf. \n", temp-(int)temp);
#endif
        if ((temp >0) || (i == 0))
            blockNO_file = i/16;
        else
            blockNO_file = i/16-1;
#ifdef UL_DEBUG
//        printf("remove_dir: BlockNO_file is %d.\n", blockNO_file);
#endif
        if (blockNO_file>=0 && blockNO_file<=7) {
            read_blockNO = Inode->BlockPointer[blockNO_file];
            //read_dir_entry(&rd[last_entry_blockNO*RD_BLOCK_SIZE+((last_entryNO-1)%16)*16], last_entry);
            read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+i*ENTRY_SIZE], DeleteEntry);
        }
        else if (blockNO_file>7 && blockNO_file<=7+64) {
            read_block_tableNO = Inode->BlockPointer[8];
            read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+(blockNO_file-8)*4));
            read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+((i-8*16)%16)*ENTRY_SIZE], DeleteEntry);
        }
        else if (blockNO_file>7+64 && blockNO_file<=7+64+64*64) {
            read_double_tableNO = Inode->BlockPointer[9];
            read_block_tableNO = *((uint32_t*)(rd+read_double_tableNO*RD_BLOCK_SIZE+((blockNO_file-(8+64))/64)*4));
            read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+((blockNO_file-(8+64))%64)*4));
            read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+((i-(8+64)*16)%16)*ENTRY_SIZE], DeleteEntry);
        }
        else return(-1);
        
        read_inode(rd, DeleteEntry->InodeNo, DeleteInode);
        if (DeleteInode->type == 1)
        {
#ifdef UL_DEBUG
//    printf("remove_dir: Delete one file.\n");
#endif
            remove_file(rd, InodeNO, DeleteEntry->InodeNo, DeleteEntry->filename);
        }
        else 
        {
#ifdef UL_DEBUG
//    printf("remove_dir: Delete one dir.\n");
#endif
            remove_dir(rd, InodeNO, DeleteEntry->InodeNo, DeleteEntry->filename);
        }
    }
    // Clear the inode bitmap and data bitmap of the selected file
    clr_inode_bitmap(rd, InodeNO);
    temp = Inode->size%RD_BLOCK_SIZE;
    if (temp > 0 || (Inode->size==0))
        blockNO = (Inode->size)/RD_BLOCK_SIZE;
    else 
        blockNO = (Inode->size)/RD_BLOCK_SIZE-1;
    
    for (i=0;i<blockNO;i++)
    {
        if (i>=0 && i<=7) {
            read_blockNO=Inode->BlockPointer[i];
            clr_bitmap(rd,read_blockNO);
        }
        else if (i>7 && i<=7+64) {
            read_block_tableNO = Inode->BlockPointer[8];
            read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+(i-8)*4));
            clr_bitmap(rd,read_blockNO);
            clr_bitmap(rd,read_block_tableNO);
        }
        else if (i>7+64 && i<=7+64+64*64) {
            read_double_tableNO = Inode->BlockPointer[9];
            read_block_tableNO = *((uint32_t*)(rd+read_double_tableNO*RD_BLOCK_SIZE+((i-(8+64))/64)*4));
            read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+((i-(8+64))%64)*4));
            clr_bitmap(rd,read_blockNO);
            clr_bitmap(rd,read_block_tableNO);
            clr_bitmap(rd,read_double_tableNO);
        }
    }
    
    // Find the entry that need to be deleted in the Parent directory

    blockNO = (ParentInode->size%RD_BLOCK_SIZE)?(ParentInode->size/RD_BLOCK_SIZE+1):ParentInode->size/RD_BLOCK_SIZE;
    entryNO = ParentInode->size/ENTRY_SIZE;
#ifdef UL_DEBUG
//    printf("remove_dir: The total entry NO is %d.\n", entryNO);
#endif
    for (i=0;i<blockNO;i++) {
        if (i>=0 && i<=7) {
            for (j=0;j<16;j++) {
                read_blockNO=ParentInode->BlockPointer[i];
                read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
#ifdef UL_DEBUG
//                    printf("remove_dir: The entry name is %s.\n", ParentDirEntry->filename);
#endif
                if (strcmp(ParentDirEntry->filename, name)==0){
                    deleted_blockNO=i;
                    deleted_entryNO=i*16+j+1;
#ifdef UL_DEBUG
//                    printf("remove_dir: The entry need to be deleted is %d.\nFrom the first 8 blocks.\n", deleted_entryNO);
#endif
                    break;
                }
            }
        }
        else if (i>7 && i<=7+64) {
            for (j=0;j<16;j++) {
                read_block_tableNO=ParentInode->BlockPointer[8];
                read_blockNO=*((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+(i-8)*4));
                read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    deleted_blockNO=i;
                    deleted_entryNO=i*16+j+1;
                    break;
                }
            }
        }
        else if (i>7+64 && i<=7+64+64*64) {
            for (j=0;j<16;j++) {
                read_double_tableNO=ParentInode->BlockPointer[9];
                read_block_tableNO=*((uint32_t*)(rd+read_double_tableNO*RD_BLOCK_SIZE+((i-(8+64))/64)*4));
                read_blockNO=*((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+((i-(8+64))%64)*4));
                read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    deleted_blockNO=i;
                    deleted_entryNO=i*16+j+1;
                    break;
                }
            }
        }
    }
    // Now ParentDirEntry should contain the entry that should be deleted
    int delete_flag=delete_dir_entry(rd, ParentInode, ParentInodeNO, deleted_entryNO);
    if (delete_flag == -1)
        return(-1);
    partial_update_superblock(rd);
#ifndef UL_DEBUG
		vfree(SuperBlock);
		vfree(Inode);
		vfree(ParentInode);
		vfree(DeleteInode);
		vfree(ParentDirEntry);
		vfree(DeleteEntry);
#endif

    return 0; // -1 means removal fails
}

//void switch_block (uint8_t* rd, int DirInodeNO, char)
int delete_dir_entry(uint8_t* rd, struct rd_inode* ParentInode, uint16_t ParentInodeNO, int deleted_entryNO)
{
    int last_entryNO;
    int last_entry_blockNO;
    int last_entry_block_tableNO;
    int last_entry_double_tableNO;

    int deleted_entry_blockNO;
    int deleted_entry_block_tableNO;
    int deleted_entry_double_tableNO;

    struct dir_entry* last_entry;
#ifdef UL_DEBUG   
	if(!(last_entry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
#endif
#ifndef UL_DEBUG   
	if(!(last_entry=(struct dir_entry*)vmalloc(sizeof(struct dir_entry)))){
		printk("<1> No mem space!\n");
		return (-1);
	}
#endif

    last_entryNO=(ParentInode->size)/ENTRY_SIZE;

    if (last_entryNO>=1 && last_entryNO<=8*16 )
    {
        last_entry_blockNO = ParentInode->BlockPointer[(int)(last_entryNO-1)/16];
#ifdef UL_DEBUG
//        printf("The last entry %d is in block#%d.\n", last_entryNO, last_entry_blockNO);
#endif
        if (last_entryNO%16 == 1)
            clr_bitmap(rd, last_entry_blockNO);
    }
    else if (last_entryNO>8*16 && last_entryNO<=(8+64)*16)
    {
        last_entry_block_tableNO = ParentInode->BlockPointer[8]; 
        last_entry_blockNO = *((uint32_t*)(rd+last_entry_block_tableNO*RD_BLOCK_SIZE+((int)((last_entryNO-1)/16)-8)*4));
#ifdef UL_DEBUG
//        printf("The last entry %d is in block#%d.\n", last_entryNO, last_entry_blockNO);
#endif
        if (last_entryNO%16 == 1)
            clr_bitmap(rd, last_entry_blockNO);
    }
    else if (last_entryNO>(8+64)*16 && last_entryNO<=(8+64+64*64)*16)
    {
        last_entry_double_tableNO = ParentInode->BlockPointer[9];
        last_entry_block_tableNO = *((uint32_t*)(rd+last_entry_double_tableNO*RD_BLOCK_SIZE+(((last_entryNO-1)/16-(8+64))/64)*4));
        last_entry_blockNO = *((uint32_t*)(rd+last_entry_block_tableNO*RD_BLOCK_SIZE+(((last_entryNO-1)/16-(8+64))%64)*4));
#ifdef UL_DEBUG
//        printf("The last entry %d is in block#%d.\n", last_entryNO, last_entry_blockNO);
#endif
        if (last_entryNO%16 == 1)
            clr_bitmap(rd, last_entry_blockNO);
    }
    else return(-1);

    if (last_entryNO == deleted_entryNO)
    {
#ifdef UL_DEBUG
//        printf("Need to delete the last entry\n");
#endif
        clear_dir_entry(&rd[last_entry_blockNO*RD_BLOCK_SIZE+((last_entryNO-1)%16)*16]);
        ParentInode->size -= 16;
        update_inode(rd, ParentInodeNO, ParentInode);
        return 0;
    }
    else
    {
        if (deleted_entryNO>=1 && deleted_entryNO<=8*16)
        {
            deleted_entry_blockNO = ParentInode->BlockPointer[(int)((deleted_entryNO-1)/16)];
#ifdef UL_DEBUG
//            printf("The delete entry %d is in block#%d.\n", deleted_entryNO, deleted_entry_blockNO);
#endif
        }
        else if (deleted_entryNO>8*16 && deleted_entryNO<=(8+64)*16)
        {
            deleted_entry_block_tableNO = ParentInode->BlockPointer[8]; 
            deleted_entry_blockNO = *((uint32_t*)(rd+deleted_entry_block_tableNO*RD_BLOCK_SIZE+((int)((deleted_entryNO-1)/16)-8)*4));
#ifdef UL_DEBUG
//            printf("The delete entry %d is in block#%d.\n", deleted_entryNO, deleted_entry_blockNO);
#endif
        }
        else if (deleted_entryNO>(8+64)*16 && deleted_entryNO<=(8+64+64*64)*16)
        {
            deleted_entry_double_tableNO = ParentInode->BlockPointer[9];
            deleted_entry_block_tableNO = *((uint32_t*)(rd+deleted_entry_double_tableNO*RD_BLOCK_SIZE+(((deleted_entryNO-1)/16-(8+64))/64)*4));
            deleted_entry_blockNO = *((uint32_t*)(rd+deleted_entry_block_tableNO*RD_BLOCK_SIZE+(((deleted_entryNO-1)/16-(8+64))%64)*4));
#ifdef UL_DEBUG
//            printf("The delete entry %d is in block#%d.\n", deleted_entryNO, deleted_entry_blockNO);
#endif
        }
        else 
        {
#ifdef UL_DEBUG
//            printf("delete_dir_entry: Where are you? The delete entry is %d.\n", deleted_entryNO);
#endif
            return(-1);
        }
        read_dir_entry(&rd[last_entry_blockNO*RD_BLOCK_SIZE+((last_entryNO-1)%16)*16], last_entry);
        write_dir_entry(&rd[deleted_entry_blockNO*RD_BLOCK_SIZE+((deleted_entryNO-1)%16)*16], last_entry);
        clear_dir_entry(&rd[last_entry_blockNO*RD_BLOCK_SIZE+((last_entryNO-1)%16)*16]);
        ParentInode->size -= 16;
        update_inode(rd, ParentInodeNO, ParentInode);
#ifndef UL_DEBUG
		vfree(last_entry);
#endif
        return 0;
    } 

}

int find_same_name(uint8_t* rd, struct rd_inode* ParentInode, char* name)
{

    int blockNO;
    int read_blockNO;
    int read_block_tableNO;
    int read_double_tableNO;
    int i,j;
    struct dir_entry* ParentDirEntry;
#ifdef UL_DEBUG
	if(!(ParentDirEntry=(struct dir_entry*)malloc(sizeof(struct dir_entry)))){
		fprintf(stderr, "No space \n");
		exit(-1);
	}
#endif
#ifndef UL_DEBUG
	if(!(ParentDirEntry=(struct dir_entry*)vmalloc(sizeof(struct dir_entry)))){
		printk( "No space \n");
		return 0;
	}
#endif
    blockNO = (ParentInode->size%RD_BLOCK_SIZE)?(ParentInode->size/RD_BLOCK_SIZE+1):ParentInode->size/RD_BLOCK_SIZE;
#ifdef UL_DEBUG
	printf("comparing\n");

#endif
    for (i=0;i<blockNO;i++) {
		//printf("i=%d, j=%d\n ",i,j, ParentDirEntry-);
        if (i>=0 && i<=7) {
            for (j=0;j<16;j++) {
                read_blockNO = ParentInode->BlockPointer[i];
                read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
				//printf("i=%d ,j=%d, %s, %s\n",i,j,ParentDirEntry->filename, name);
                if (strcmp(ParentDirEntry->filename, name)==0){
                    return(-1);
                }
            }
        }
        else if (i>7 && i<=7+64) {
            for (j=0;j<16;j++) {
                read_block_tableNO = ParentInode->BlockPointer[8];
		//		printf("1 %d \n",read_block_tableNO);
				read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+(i-8)*4));
          //      printf("2 %d %d \n",read_blockNO,read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE);
				read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
            //    printf("3\n");
				if (strcmp(ParentDirEntry->filename, name)==0)
                    return(-1);
            }
        }
        else if (i>7+64 && i<=7+64+64*64) {
            for (j=0;j<16;j++) {
                read_double_tableNO = ParentInode->BlockPointer[9];
                read_block_tableNO = *((uint32_t*)(rd+read_double_tableNO*RD_BLOCK_SIZE+((i-(8+64))/64)*4));
                read_blockNO = *((uint32_t*)(rd+read_block_tableNO*RD_BLOCK_SIZE+((i-(8+64))%64)*4));
                read_dir_entry(&rd[read_blockNO*RD_BLOCK_SIZE+j*ENTRY_SIZE], ParentDirEntry);
                if (strcmp(ParentDirEntry->filename, name)==0)
                    return(-1);
            }
        }
    }
#ifndef UL_DEBUG
	vfree(ParentDirEntry);
#endif
    return 0;
}
int chmod_reg_file(uint8_t* rd, uint16_t InodeNO, int mode)
{
	struct rd_inode* Inode;
	if (!(Inode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode))))
	{
		printk("No mem space!\n");
		return 0;
	}

	read_inode(rd, InodeNO, Inode);
	printk("The previous Inode mode is %d.\n",Inode->mode);
	Inode->mode = mode;
	update_inode(rd, InodeNO, Inode);
	printk("The new Inode mode is %d.\n",Inode->mode);
	vfree(Inode);
	return 0;
}
int check_mode_file(uint8_t* rd, uint16_t InodeNO)
{
	int mode;
	struct rd_inode* Inode;
	if (!(Inode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode))))
	{
		printk("No mem space!\n");
		return 0;
	}

	read_inode(rd, InodeNO, Inode);
	mode = Inode->mode;
	vfree(Inode);
	return mode;
}
