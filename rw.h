#ifndef RW_H
#define RW_H

//#define KL_DEBUG
#ifndef UL_DEBUG
#include<linux/vmalloc.h>
#include<linux/types.h>
#endif

#ifdef UL_DEBUG
#include<stdint.h>
#endif

#include"file_func.h"
#include"ramdisk_struct.h"
#include"constant.h"
#include"ramdisk_struct.h"

uint8_t* file_byte_locate(uint8_t* rd, uint16_t inodeNO, int pos);

uint8_t* file_byte_allocate(uint8_t* rd, uint16_t inodeNO);

int read_ramdisk(uint8_t* rd, uint16_t inodeNO, int pos, uint8_t* buf, int length);

int write_ramdisk(uint8_t* rd, uint16_t inodeNO, int pos, uint8_t* buf, int length);

int readdir(uint8_t* rd, uint16_t inodeNO, int pos, uint8_t* buf);




#endif
