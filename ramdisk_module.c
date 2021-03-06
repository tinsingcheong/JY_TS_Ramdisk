
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h> /* error codes */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/semaphore.h>

//#define KL_DEBUG
//#include "constant.h"
#include "file_func.h"
#include "ramdisk.h"
//#include "ramdisk_struct.h"
#include "rw.h"

#define RD_READ_WRITE 0
#define RD_READ_ONLY  1
#define RD_WRITE_ONLY 2

MODULE_LICENSE("GPL");

/* attribute structures */
struct ramdisk_ops_arg_list {
  uint8_t* pathname;
  int pathname_len;
  int mode;
  uint16_t inodeNO;
  int pos;
  uint8_t* buf;
  int length;
  int ret;
};



//#define IOCTL_TEST _IOW(0, 6, struct ioctl_test_t)
//#define CHAR_GET _IOR(0, 7, char)

#define RD_CREATE  _IOWR(0,0,struct ramdisk_ops_arg_list)
#define RD_MKDIR   _IOWR(1,0,struct ramdisk_ops_arg_list)
#define RD_OPEN    _IOWR(2,0,struct ramdisk_ops_arg_list)
#define RD_READ    _IOWR(3,0,struct ramdisk_ops_arg_list)
#define RD_WRITE   _IOWR(4,0,struct ramdisk_ops_arg_list)
#define RD_LSEEK   _IOWR(5,0,struct ramdisk_ops_arg_list)
#define RD_UNLINK  _IOWR(6,0,struct ramdisk_ops_arg_list)
#define RD_READDIR _IOWR(7,0,struct ramdisk_ops_arg_list)
#define RD_CHMOD   _IOWR(8,0,struct ramdisk_ops_arg_list)
#define RD_SYNC    _IOWR(9,0,struct ramdisk_ops_arg_list)
#define RD_RESTORE _IOWR(10,0,struct ramdisk_ops_arg_list)

static uint8_t* rd;

char my_getchar ( void );

static int ramdisk_ioctl(struct rd_inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg);

static struct file_operations ramdisk_proc_operations;

static struct proc_dir_entry *proc_entry;

struct semaphore mutex;

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
  sema_init(&mutex,1);


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
  vfree(rd);
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
	if(i!=0){
		for(j=0;j<i;j++){
			parent[j]=path[j];
		}
		parent[j]='\0';
	}
	else{
		parent[0]='/';
		parent[1]='\0';
	}


}

/***
 * ioctl() entry point...
 */
static int ramdisk_ioctl(struct rd_inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	struct ramdisk_ops_arg_list ioc;
	char path[1024];
	char parent[1024];
	char File[14];
	uint16_t ParentInodeNO;
	uint16_t InodeNO;
	uint8_t* buf;
	int size;
	int mode;
	switch (cmd){

	case RD_CREATE:
		down_interruptible(&mutex);

		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		copy_from_user(path, ioc.pathname, ioc.pathname_len);
	//	printk("<1> pathname is %s and %d\n",path,ioc.pathname_len);
		seperate_path(path,ioc.pathname_len,parent,File);
	//	printk("<1> After seperating the parent is %s and File is %s\n",parent, File);
		ParentInodeNO=search_file(rd,parent);
	//	printk("<1> The parent inode No is %d\n",ParentInodeNO);
		ioc.ret=create_file(rd,ParentInodeNO,File, ioc.mode);
	//	printk("<1> The create file ret value is %d\n", ioc.ret);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		
		up(&mutex);
		break;

	case RD_MKDIR:
		down_interruptible(&mutex);

		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		copy_from_user(path, ioc.pathname, ioc.pathname_len);
		printk("<1> pathname is %s and %d\n",path,ioc.pathname_len);

		seperate_path(path,ioc.pathname_len,parent,File);
		printk("<1> After seperating the parent is %s and File is %s\n",parent, File);

		ParentInodeNO=search_file(rd,parent);
		printk("<1> The parent inode No is %d\n",ParentInodeNO);

		ioc.ret=create_dir(rd,ParentInodeNO,File);
		printk("<1> The create dir ret value is %d\n", ioc.ret);

		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		
		up(&mutex);

		break;

	case RD_OPEN:
		down_interruptible(&mutex);

		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		copy_from_user(path, ioc.pathname, ioc.pathname_len);
		printk("<1> opening file %s\n", path);
		ioc.ret=search_file(rd,path);
		printk("<1> RD_OPEN: file inodeNO is %d.\n", ioc.ret);
		mode=check_mode_file(rd,ioc.ret);
		printk("<1> RD_OPEN: after check the file mode is %d, the flags is %d.\n", mode, ioc.mode);
		if ((mode == RD_READ_ONLY) && (ioc.mode != RD_READ_ONLY))
		  ioc.ret = -1;
		else if ((mode == RD_WRITE_ONLY) && (ioc.mode != RD_WRITE_ONLY))
		  ioc.ret = -1;
		else 
		  ioc.ret=search_file(rd,path);
		
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));

		up(&mutex);
		break;

	case RD_READ:
		down_interruptible(&mutex);

		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		ioc.ret=read_ramdisk(rd, ioc.inodeNO, ioc.pos, ioc.buf, ioc.length);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));

		up(&mutex);
		break;

	case RD_WRITE:
		down_interruptible(&mutex);

		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		ioc.ret=write_ramdisk(rd, ioc.inodeNO, ioc.pos, ioc.buf, ioc.length);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		
		up(&mutex);
		break;

	case RD_LSEEK:
		down_interruptible(&mutex);

		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
        size=get_file_size(rd, ioc.inodeNO);
		if(size>ioc.length){
			//ioc.length=ioc.length;
			ioc.ret=0;
		}
		else if(size<=ioc.length && size>=0){
			ioc.length=-1;//end of file
			ioc.ret=-1;
		}
		else if(size<0){
			ioc.ret=-1;
		}
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		
		up(&mutex);
		break;

	case RD_UNLINK:
		down_interruptible(&mutex);

		printk("<1> start unlinking\n");
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		copy_from_user(path, ioc.pathname, ioc.pathname_len);
		int inodeNO=search_file(rd,path);
		printk("<1> %s inode is %d\n",path,inodeNO);
		if(inodeNO<0){
			ioc.ret=-1;
			copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));

			up(&mutex);
			break;
		}
		seperate_path(path,ioc.pathname_len,parent,File);
		ParentInodeNO=search_file(rd,parent);
		printk("<1> Parent %s inode is %d\n",parent, ParentInodeNO);
		if(ParentInodeNO<0){
			ioc.ret=-1;
			copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
			
			up(&mutex);
			break;
		}

		struct rd_inode* inode;
		if(!(inode=(struct rd_inode*)vmalloc(sizeof(struct rd_inode)))){
			printk("<1> No enough space\n");
			return 1;
		}
		printk("<1> reading inode %d\n",inodeNO);
		read_inode(rd,inodeNO,inode);
		if(inode->type==1){
			printk("<1> start removing file\n");
			ioc.ret=remove_file(rd,ParentInodeNO,inodeNO,File);
			printk("<1> finish removing file ret val is %d\n",ioc.ret);
			copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
			
			up(&mutex);
			break;
		}
		if(inode->type==0){
			printk("<1> start removing dir\n");

			ioc.ret=remove_dir(rd,ParentInodeNO,inodeNO,File);
			printk("<1> finish removing dir ret val is %d\n",ioc.ret);

			copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
			
			up(&mutex);
			break;
		}
		break;
	case RD_READDIR:
		down_interruptible(&mutex);
		
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		printk("<1> reading the dir whose inode is %d and pos is %d\n",ioc.inodeNO,ioc.pos);
		ioc.ret=readdir(rd, ioc.inodeNO, ioc.pos, ioc.buf);
		printk("<1> the ret value is %d\n",ioc.ret);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));
		
		up(&mutex);
		break;
	case RD_CHMOD:
		down_interruptible(&mutex);

		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, sizeof(struct ramdisk_ops_arg_list));
		copy_from_user(path, ioc.pathname, ioc.pathname_len);
		printk("<1> opening file %s\n", path);
		InodeNO=search_file(rd,path);
		printk("<1> The InodeNO is %u\n", InodeNO);
		ioc.ret=chmod_reg_file(rd,InodeNO,ioc.mode);
		printk("<1> The changed mode is %d;The chmod ret value is %d\n", ioc.mode, ioc.ret);
		copy_to_user((struct ramdisk_ops_arg_list*)arg, &ioc, sizeof(struct ramdisk_ops_arg_list));

		up(&mutex);
		break;
	case RD_SYNC:
		down_interruptible(&mutex);

		copy_to_user((uint8_t*)arg, rd, sizeof(uint8_t)*RAMDISK_SIZE);
		up(&mutex);
		break;

	case RD_RESTORE:
		down_interruptible(&mutex);

		copy_from_user(rd, (uint8_t*)arg, sizeof(uint8_t)*RAMDISK_SIZE);
		up(&mutex);
		break;

	default:
		return -EINVAL;
		break;
  
	}
  
  return 0;
}

module_init(initialization_routine); 
module_exit(cleanup_routine); 
