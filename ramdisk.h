#ifndef RAMDISK_H
#define RAMDISK_H

#define KL_DEBUG
#include "ramdisk_struct.h"
#include "constant.h"
//#define UL_DEBUG //for user level debugging
//#define KL_DEBUG


#include<linux/types.h>


#ifdef UL_DEBUG
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#endif

void update_superblock(uint8_t* rd, struct rd_super_block* SuperBlock);
void partial_update_superblock(uint8_t* rd);
void read_superblock(uint8_t* rd, struct rd_super_block* SuperBlock);
void update_inode(uint8_t* rd, uint16_t NodeNO, struct rd_inode* Inode);

void read_inode(uint8_t* rd, uint16_t NodeNO, struct rd_inode* Inode);
void set_bitmap(uint8_t* rd, int BlockNO);
void clr_bitmap(uint8_t* rd, int BlockNO);
int find_next_free_block(uint8_t* rd);
uint16_t find_next_free_inode(uint8_t* rd);
int bitmap_sum_up(uint8_t* rd);
void set_inode_bitmap(uint8_t* rd, uint16_t InodeNO);
void clr_inode_bitmap(uint8_t* rd, uint16_t InodeNO);
int inode_bitmap_sum_up(uint8_t* rd);
void read_dir_entry(uint8_t* ptr, struct dir_entry* DirEntry);
void write_dir_entry(uint8_t* ptr, struct dir_entry* DirEntry);
void clear_dir_entry(uint8_t* ptr);
uint8_t* ramdisk_init(void);
int search_file(uint8_t* rd, char* path);
#endif

