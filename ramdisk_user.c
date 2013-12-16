#include <stdio.h>
#include<stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ramdisk_user.h"
extern struct file_object fd_table[1024];

int ioctl_rd_fd; 
int rd_create (char *pathname)
{
    struct rd_ops_arg_list rd_args;
    //printf("In rd_create already!");

    if (!ioctl_rd_fd)
        ioctl_rd_fd = open("/proc/ramdisk", O_RDONLY);
    //printf("Open ramdisk!");

    rd_args.pathname = pathname;
    rd_args.path_len = strlen(pathname)+1;

    //printf("Create: Pass the struct to kernel!");
    ioctl(ioctl_rd_fd, RD_CREATE, &rd_args);
    //printf("Create: Return value is %d!", rd_args.ret);
    return rd_args.ret; 
}

int rd_mkdir (char *pathname)
{
    struct rd_ops_arg_list rd_args;
    printf("In rd_mkdir already!");

    if (!ioctl_rd_fd)
        ioctl_rd_fd = open("/proc/ramdisk", O_RDONLY);
    printf("Open ramdisk!");

    rd_args.pathname = pathname;
    rd_args.path_len = strlen(pathname)+1;

    ioctl(ioctl_rd_fd, RD_MKDIR, &rd_args);
    return rd_args.ret; 
}

int rd_open (char *pathname)
{
    struct rd_ops_arg_list rd_args;
    int fd;
    printf("In rd_open already!");

    if (!ioctl_rd_fd)
        ioctl_rd_fd = open("/proc/ramdisk", O_RDONLY);
    printf("Open ramdisk!");

    rd_args.pathname = pathname;
    rd_args.path_len = strlen(pathname)+1;
    
    ioctl(ioctl_rd_fd, RD_OPEN, &rd_args);

    fd = fd_search_fd(fd_table, rd_args.inodeNO);
    if (fd == -1){ 
        // If the file is not registered in the fd table
        fd = fd_find_free_fd(fd_table);
        fd_table[fd].valid = 1;
        fd_table[fd].file_pos = 0;
        fd_table[fd].inodeNO = rd_args.inodeNO;
        strcpy(fd_table[fd].pathname,pathname);
        return fd;
    }
    else 
        return fd;
}

int rd_close (int fd)
{
    struct rd_ops_arg_list rd_args;
    printf("In rd_close already!");

    if (!ioctl_rd_fd)
        ioctl_rd_fd = open("/proc/ramdisk", O_RDONLY);
    printf("Open ramdisk!");

    if (fd_table[fd].valid == 0)
        return(-1); // File is not open
    else
        fd_table[fd].valid = 0;
    return 0; 
}

int rd_read (int fd, char *address, int num_bytes)
{
    struct rd_ops_arg_list rd_args;
    printf("In rd_read already!");

    if (!ioctl_rd_fd)
        ioctl_rd_fd = open("/proc/ramdisk", O_RDONLY);
    printf("Open ramdisk!");

    if (fd_table[fd].valid == 0)
        return(-1); // File is not open

    rd_args.inodeNO = fd_table[fd].inodeNO;
    rd_args.buf     = address;
    rd_args.buf_len = num_bytes;
    rd_args.pos     = fd_table[fd].file_pos;
    ioctl(ioctl_rd_fd, RD_READ, &rd_args);

    fd_table[fd].file_pos += rd_args.ret;
    return rd_args.ret; 
}

int rd_write (int fd, char *address, int num_bytes)
{
    struct rd_ops_arg_list rd_args;
    printf("In rd_write already!");

    if (!ioctl_rd_fd)
        ioctl_rd_fd = open("/proc/ramdisk", O_RDONLY);
    printf("Open ramdisk!");

    if (fd_table[fd].valid == 0)
        return(-1); // File is not open

    rd_args.inodeNO = fd_table[fd].inodeNO;
    rd_args.buf     = address;
    rd_args.buf_len = num_bytes;
    rd_args.pos     = fd_table[fd].file_pos;
    ioctl(ioctl_rd_fd, RD_WRITE, &rd_args);

    fd_table[fd].file_pos += rd_args.ret;
    return rd_args.ret;
}

int rd_lseek (int fd, int offset)
{
    struct rd_ops_arg_list rd_args;
    printf("In rd_lseek already!");

    if (!ioctl_rd_fd)
        ioctl_rd_fd = open("/proc/ramdisk", O_RDONLY);
    printf("Open ramdisk!");

    if (fd_table[fd].valid == 0)
        return(-1); // File is not open

    rd_args.inodeNO = fd_table[fd].inodeNO;
    rd_args.buf_len = offset;
    rd_args.pos     = fd_table[fd].file_pos;
    ioctl(ioctl_rd_fd, RD_LSEEK, &rd_args);

    return rd_args.ret; // Success: Return the new position or the end of the file
}

int rd_unlink (char *pathname)
{
    struct rd_ops_arg_list rd_args;
    printf("In rd_unlink already!");
	fflush(stdout);
    if (!ioctl_rd_fd)
        ioctl_rd_fd = open("/proc/ramdisk", O_RDONLY);
    printf("Open ramdisk!");
	fflush(stdout);
    if (fd_find_pathname(fd_table, pathname) == 0)
        return(-1);

    rd_args.pathname = pathname;
    rd_args.path_len = strlen(pathname)+1;

    ioctl(ioctl_rd_fd, RD_UNLINK, &rd_args);

    return rd_args.ret;
}

int rd_readdir (int fd, char *address)
{
    struct rd_ops_arg_list rd_args;
    printf("In rd_readdir already!");

    if (!ioctl_rd_fd)
        ioctl_rd_fd = open("/proc/ramdisk", O_RDONLY);
    printf("Open ramdisk!");

    rd_args.inodeNO = fd_table[fd].inodeNO;
    rd_args.buf     = address;
    return rd_args.ret;
}

int fd_search_fd(struct file_object* table, uint16_t InodeNO)
{
    int i;
    for (i=0;i<1024;i++)
    {
        if ((table[i].inodeNO == InodeNO) && table[i].valid == 1)
            return i;
    }
    return(-1); 
}

int fd_find_free_fd(struct file_object* table) 
{
    int i;
    for (i=0;i<1024;i++)
    {
        if (table[i].valid == 0)
            return i;
    }
    return(-1);
}

int fd_find_pathname(struct file_object* table, char *pathname)
{
    int i;
    for (i=0;i<1024;i++)
    {
		//printf("i=%d\n",i);
		fflush(stdout);
        if ((strcmp(table[i].pathname, pathname)==0) && table[i].valid == 1)
            return 0;
    }
    return(-1);
}

