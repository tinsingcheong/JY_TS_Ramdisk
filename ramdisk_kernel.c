
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h> /* error codes */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/types.h>

#include "constant.h"
#include "file_func.h"
#include "ramdisk.h"
#include "ramdisk_struct.h"
#include "rw.h"



MODULE_LICENSE("GPL");

/* attribute structures */
struct ramdisk_ops_arg_list {
  uint8_t* pathname;
  int pathname_len;

  uint16_t inodeNO;
  int pos;
  uint8_t* buf;
  int length;
  int ret;
};



//#define IOCTL_TEST _IOW(0, 6, struct ioctl_test_t)
//#define CHAR_GET _IOR(0, 7, char)

#define CREATE  _IOWR(0,0,struct ramdisk_ops_arg_list)
#define MKDIR   _IOWR(1,0,struct ramdisk_ops_arg_list)
#define OPEN    _IOWR(2,0,struct ramdisk_ops_arg_list)
#define READ    _IOWR(3,0,struct ramdisk_ops_arg_list)
#define WRITE   _IOWR(4,0,struct ramdisk_ops_arg_list)
#define UNLINK  _IOWR(5,0,struct ramdisk_ops_arg_list)
#define READDIR _IOWR(6,0,struct ramdisk_ops_arg_list)

static uint8_t* rd;

char my_getchar ( void );

static int ramdisk_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg);

static struct file_operations ramdisk_proc_operations;

static struct proc_dir_entry *proc_entry;

static int __init initialization_routine(void) {
  printk("<1> Loading module\n");

  ramdisk_proc_operations.ioctl = ramdisk_ioctl;

  /* Start create proc entry */
  proc_entry = create_proc_entry("ramdisk", 0666, NULL);
  if(!proc_entry)
  {
    printk("<1> Error creating /proc entry.\n");
    return 1;
  }

  //proc_entry->owner = THIS_MODULE; <-- This is now deprecated
  proc_entry->proc_fops = &ramdisk_proc_operations;

  //init ramdisk
  rd=ramdisk_init();


  return 0;
}

/* 'printk' version that prints to active tty. */
void my_printk(char *string)
{
  struct tty_struct *my_tty;

  my_tty = current->signal->tty;

  if (my_tty != NULL) {
    (*my_tty->driver->ops->write)(my_tty, string, strlen(string));
    (*my_tty->driver->ops->write)(my_tty, "\015\012", 2);
  }
} 


static void __exit cleanup_routine(void) {

  printk("<1> Dumping module\n");
  remove_proc_entry("ramdisk", NULL);

  return;
}

static void seperate_path(char* path, int path_len, char* parent, char* file){
	int i,j;
	i=path_len-1;
	while(path[i]!='/'){
		i--;
	}
	for(j=i+1;j<path_len;j++){
		file[j-i-1]=path[j];
	}
	file[j-i-1]='\0';
	for(j=0;j<i;j++){
		parent[j]=path[j];
	}
	parent[j]='\0';

}

/***
 * ioctl() entry point...
 */
static int ramdisk_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	struct ramdisk_ops_arg_list ioc;
	char path[1024];
	char parent[1024];
	char file[14];
	int ParentInodeNO;
	uint8_t* buf;
	switch (cmd){

	case CREATE:
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		copy_from_user(path, ioc.pathname, ioc.pathname_len);
		seperate_path(path,ioc.pathname_len,parent,file);
		ParentInodeNO=search_file(rd,parent);
		ioc.ret=create_file(rd,ParentInodeNO,file);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		break;

	case MKDIR:
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		copy_from_user(path, ioc.pathname, ioc.pathname_len);
		seperate_path(path,ioc.pathname_len,parent,file);
		ParentInodeNO=search_file(rd,parent);
		ioc.ret=create_dir(rd,ParentInodeNO,file);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		break;

	case OPEN:
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		copy_from_user(path, ioc.pathname, ioc.pathname_len);
		ioc.ret=search_file(rd,path);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		break;

	case READ:
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		ioc.ret=read_ramdisk(rd, ioc.inodeNO, ioc.pos, ioc.buf, ioc.length);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		break;

	case WRITE:
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		ioc.ret=write_ramdisk(rd, ioc.inodeNO, ioc.pos, ioc.buf, ioc.length);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		break;

	case UNLINK:
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		copy_from_user(path, ioc.pathname, ioc.pathname_len);
		int inodeNO=search_file(rd,path);
		if(inodeNO<0){
			ioc.ret=-1;
			copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
			break;
		}
		seperate_path(path,ioc.pathname_len,parent,file);
		ParentInodeNO=search_file(rd,parent);
		if(ParentInodeNO<0){
			ioc.ret=-1;
			copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
			break;
		}

		struct inode* inode;
		if(!(inode=(struct inode*)vmalloc(sizeof(struct inode)))){
			printfk("No enough space\n");
			return 1;
		}
		read_inode(rd,inodeNO,inode);
		if(inode->type==1){
			ioc.ret=remove_file(rd,ParentInodeNO,inodeNO,file);
			copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
			break;
		}
		if(inode->type==0){
			ioc.ret=remove_dir(rd,ParentInodeNO,inodeNO,file);
			copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
			break;
		}
		break;
	case READDIR:
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));

		ioc.ret=readdir(rd, ioc.inodeNO, ioc.pos, ioc.buf);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		break;

	default:
		return -EINVAL;
		break;
  
	}
  
  return 0;
}

module_init(initialization_routine); 
module_exit(cleanup_routine); 
