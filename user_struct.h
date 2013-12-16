#ifndef USER_STRUCT_H
#define USER_STRUCT_H

struct file_object{
	int file_pos;
	int inode_ptr;

};

struct fd_table_entry{
	int fd;
	struct file_object;
	struct fd_table_entry* next;
};

struct ramdisk_ops_arg_list {
    uint8_t* pathname;
    int pathname_len;
    uint16_t inodeNO;
int 
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

#endif

