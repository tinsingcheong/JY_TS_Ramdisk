#ifndef RAMDISK_STRUCT_H
#define RAMDISK_STRUCT_H

//#define KL_DEBUG

#ifndef UL_DEBUG
#include<linux/types.h>
#endif

#ifdef UL_DEBUG
#include<stdint.h>
#endif
struct rd_super_block{
	uint16_t FreeBlockNum;
	uint16_t FreeInodeNum;
	uint8_t InodeBitmap[128];
};

struct rd_inode{
	uint8_t type;//1 is regular file, 0 is dir file
    // Access rights begin
    uint8_t write; // 0: writable; 1: couldnt be written
    uint8_t read; // 0: readable; 1: couldnt be read
    // Access rights end
	uint32_t size;
	uint32_t BlockPointer[10];
};

struct mode_t{
    uint8_t write;
    uint8_t read;
};
struct rd_path{
	char filename[14];
	struct rd_path* next;
};

struct dir_entry{
	char filename[14];
	uint16_t InodeNo;
};
#endif

