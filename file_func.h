#include <stdint.h>
#ifndef FILE_FUNC_H
#define FILE_FUNC_H
int write_file (uint8_t* rd, uint16_t InodeNO, int pos, char* string, int length);
int read_file (uint8_t* rd, uint16_t InodeNO, int pos);
int read_dir (uint8_t* rd, uint16_t InodeNO);
int get_file_size (uint8_t* rd, uint16_t InodeNO);
int create_file (uint8_t* rd, uint16_t ParentInodeNO, char* name);
int create_dir (uint8_t* rd, uint16_t ParentDirInode, char* name);
int remove_file (uint8_t* rd, uint16_t ParentInodeNO, uint16_t InodeNO, char* name);
int remove_dir (uint8_t* rd, uint16_t ParentInodeNO, uint16_t InodeNO, char* name);
int delete_dir_entry(uint8_t* rd, struct inode* Inode, int delete_blockNO);
#endif
