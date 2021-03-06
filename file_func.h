
#ifndef FILE_FUNC_H
#define FILE_FUNC_H

#include "ramdisk_struct.h"

//#define KL_DEBUG
#ifndef UL_DEBUG
#include <linux/types.h>
#endif

#ifdef UL_DEBUG
#include <stdint.h>
#endif

int get_file_size (uint8_t* rd, uint16_t InodeNO);
int create_file (uint8_t* rd, uint16_t ParentInodeNO, char* name, int mode);
int create_dir (uint8_t* rd, uint16_t ParentDirInode, char* name);
int remove_file (uint8_t* rd, uint16_t ParentInodeNO, uint16_t InodeNO, char* name);
int remove_dir (uint8_t* rd, uint16_t ParentInodeNO, uint16_t InodeNO, char* name);
int delete_dir_entry(uint8_t* rd, struct rd_inode* ParentInode, uint16_t ParentInodeNO, int delete_blockNO);
int find_same_name(uint8_t* rd, struct rd_inode* ParentInode,char* name);
//Bonus3
int chmod_reg_file(uint8_t* rd, uint16_t InodeNO, int mode);
int check_mode_file(uint8_t* rd, uint16_t InodeNO);
//Bonus3
#endif
