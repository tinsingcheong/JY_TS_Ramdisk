#ifndef RAMDISK_STRUCT_H
#define RAMDISK_STRUCT_H
struct super_block{
	uint16_t FreeBlockNum;
	uint16_t FreeInodeNum;
	uint8_t InodeBitmap[128];
};

struct inode{
	uint8_t type;//1 is regular file, 0 is dir file
	uint32_t size;
	uint32_t BlockPointer[10];
};

struct path{
	char filename[14];
	struct path* next;
};

struct dir_entry{
	char filename[14];
	uint16_t InodeNo;
};
#endif

