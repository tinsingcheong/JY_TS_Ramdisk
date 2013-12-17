#ifndef USER_STRUCT_H
#define USER_STRUCT_H

#include<stdint.h>

#define RD_CREATE  _IOWR(0,0,struct rd_ops_arg_list)
#define RD_MKDIR   _IOWR(1,0,struct rd_ops_arg_list)
#define RD_OPEN    _IOWR(2,0,struct rd_ops_arg_list)
#define RD_READ    _IOWR(3,0,struct rd_ops_arg_list)
#define RD_WRITE   _IOWR(4,0,struct rd_ops_arg_list)
#define RD_LSEEK   _IOWR(5,0,struct rd_ops_arg_list)
#define RD_UNLINK  _IOWR(6,0,struct rd_ops_arg_list)
#define RD_READDIR _IOWR(7,0,struct rd_ops_arg_list)
#define RD_CHMOD   _IOWR(8,0,struct rd_ops_arg_list)
#define RD_SYNC    _IOWR(9,0,struct rd_ops_arg_list)
#define RD_RESTORE _IOWR(10,0,struct rd_ops_arg_list)

#define RD_READ_WRITE 0
#define RD_READ_ONLY  1
#define RD_WRITE_ONLY 2
#define RAMDISK_SIZE  2097152

struct file_object{
    int valid;
	int file_pos;
	uint16_t inodeNO;
    char pathname[100];
};

struct file_object fd_table[1024];
/*
struct fd_table_entry{
	int fd;
	struct file_object;
	struct fd_table_entry* next;
};*/

struct rd_ops_arg_list {
    uint8_t* pathname;
    int path_len;
    uint16_t inodeNO;
    int pos;
    uint8_t* buf;
    int buf_len;
    int ret;
};

int rd_create (char *pathname);
int rd_mkdir (char *pathname);
int rd_open (char *pathname);
int rd_close (int fd);
int rd_read (int fd, char *address, int num_bytes);
int rd_write (int fd, char *address, int num_bytes);
int rd_lseek (int fd, int offset);
int rd_unlink (char *pathname);
int rd_readdir (int fd, char *address);
int rd_sync ();
int rd_restore ();

int fd_search_fd(struct file_object* table, uint16_t InodeNO);
int fd_find_free_fd(struct file_object* table);
int fd_find_pathname(struct file_object* table, char *pathname);
#endif

