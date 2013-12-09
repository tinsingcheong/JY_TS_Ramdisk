#ifndef RAMDISK_H
#define RAMDISK_H
#include "ramdisk_struct.h"
#include "constant.h"
#define UL_DEBUG //for user level debugging

#ifdef ULDEBUG
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#endif

void update_superblock(uint8_t* rd, struct super_block* SuperBlock);
void partial_update_superblock(uint8_t* rd);
void read_superblock(uint8_t* rd, struct super_block* SuperBlock);
void update_inode(uint8_t* rd, int NodeNO, struct inode* Inode);

void read_inode(uint8_t* rd, int NodeNO, struct inode* Inode);
void set_bitmap(uint8_t* rd, int BlockNO);
int find_next_free_block(uint8_t* rd);
int find_next_free_inode(uint8_t* rd);
int bitmap_sum_up(uint8_t* rd);
void set_inode_bitmap(uint8_t* rd, int InodeNO);
void clr_inode_bitmap(uint8_t* rd, int InodeNO);
int inode_bitmap_sum_up(uint8_t* rd);
void read_dir_entry(uint8_t* ptr, struct dir_entry* DirEntry);
void write_dir_entry(uint8_t* ptr, struct dir_entry* DirEntry);
uint8_t* ramdisk_init();
int search_file(uint8_t* rd, char* path);
#endif

