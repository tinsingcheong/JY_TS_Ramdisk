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





#endif

