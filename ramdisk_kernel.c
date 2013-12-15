
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


/***
 * ioctl() entry point...
 */
static int ramdisk_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	struct ramdisk_ops_arg_list ioc;
  
	switch (cmd){

	case CREATE:
		copy_from_user(&ioc, (struct ramdisk_ops_arg_list*)arg, 
		   sizeof(struct ramdisk_ops_arg_list));

		printk("<1> ioctl: call to IOCTL_TEST (%d,%c)!\n", 
	   ioc.field1, ioc.field2);

    my_printk ("Got msg in kernel\n");
    break;
  case CHAR_GET:
	C = my_getchar();
	printk("<1>%c",C);
	copy_to_user((char*)arg, &C, sizeof(char));
	break;
  
  default:
    return -EINVAL;
    break;
  }
  
  return 0;
}

module_init(initialization_routine); 
module_exit(cleanup_routine); 
