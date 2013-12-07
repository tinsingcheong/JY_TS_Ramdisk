#ifndef CONSTANT_H
#define CONSTANT_H
#define SUPERBLOCK_BASE    0
#define INODEBITMAP_BASE   4
#define INODEBITMAP_LIMIT  131
#define INODEBITMAP_SIZE   128
#define SUPERBLOCK_LIMIT   255
#define INODE_BASE         256
#define INODE_LIMIT        65791 //256+256*256-1
#define INODE_NUM          1024
#define BITMAP_BASE        65792 //256+256*256
#define BITMAP_LIMIT       66815 //256+256*256+4*256-1
#define BITMAP_SIZE        1024
#define DATA_BASE          66816 //256+256*256+4*256
#define DATA_LIMIT         2097151 //2^21-1
#define RAMDISK_SIZE       2097152 //2MB
#define BYTELEN            8
#define BLOCK_SIZE         256
#define BLOCK_NUM          8192
#endif

