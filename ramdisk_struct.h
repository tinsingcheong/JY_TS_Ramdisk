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
	uint32_t size;
	int mode;
	uint32_t BlockPointer[10];
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

