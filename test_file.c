/* 
   -- template test file for RAMDISK Filesystem Assignment.
   -- include a case for:
   -- two processes 
   -- largest number of files (should be 1024 max)
   -- largest single file (start with direct blocks [2048 bytes max], 
   then single-indirect [18432 bytes max] and finally double 
   indirect [1067008 bytes max])
   -- creating and unlinking files to avoid memory leaks
   -- each file operation
   -- error checking on invalid inputs
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include "ramdisk_user.h"

// #define's to control what tests are performed,
// comment out a test if you do not wish to perform it
//#define TEST0_SYNC
//#define TEST0_RESTORE
#define TEST_ACCESS
/*
#define TEST1
#define TEST2
#define TEST3
#define TEST4
#define TEST5
*/
// Insert a string for the pathname prefix here. For the ramdisk, it should be
// NULL
#define PATH_PREFIX ""
#define USE_RAMDISK

#ifdef USE_RAMDISK
#define CREAT   rd_create
#define OPEN    rd_open
#define WRITE   rd_write
#define READ    rd_read
#define UNLINK  rd_unlink
#define MKDIR   rd_mkdir
#define READDIR rd_readdir
#define CLOSE   rd_close
#define LSEEK   rd_lseek
#define CHMOD   rd_chmod
#define SYNC    rd_sync
#define RESTORE rd_restore

#else
#define CREAT(file)   creat(file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define OPEN(file)    open(file, O_RDWR)
#define WRITE   write
#define READ    read
#define UNLINK  unlink
#define MKDIR(path)   mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)

int my_readdir(int fd, char* str)
{
  memcpy(str, "dir_entry", sizeof("dir_entry"));
  return 1;
}

#define READDIR my_readdir
#define CLOSE   close
#define LSEEK(fd, offset)   lseek(fd, offset, SEEK_SET)

#endif

// #define's to control whether single indirect or
// double indirect block pointers are tested

#define TEST_SINGLE_INDIRECT
#define TEST_DOUBLE_INDIRECT


#define MAX_FILES 1023
#define BLK_SZ 256		/* Block size */
#define DIRECT 8		/* Direct pointers in location attribute */
#define PTR_SZ 4		/* 32-bit [relative] addressing */
#define PTRS_PB  (BLK_SZ / PTR_SZ) /* Pointers per index block */

static char pathname[80];

static char data1[DIRECT*BLK_SZ]; /* Largest data directly accessible */
static char data2[PTRS_PB*BLK_SZ];     /* Single indirect data size */
static char data3[PTRS_PB*PTRS_PB*BLK_SZ]; /* Double indirect data size */
static char addr[PTRS_PB*PTRS_PB*BLK_SZ+1]; /* Scratchpad memory */

int main () {
    
  int retval, i;
  int fd;
  int index_node_number;

  /* Some arbitrary data for our files */
  memset (data1, '1', sizeof (data1));
  memset (data2, '2', sizeof (data2));
  memset (data3, '3', sizeof (data3));

  printf("Memset Done!\n");
#ifdef TEST_ACCESS
  printf("In Access Test!\n");
  i=1;
    sprintf (pathname, PATH_PREFIX "/file%d", i);
    
    retval = CREAT (pathname, RD_READ_ONLY);
    printf("Create file %d!\n", i);
    
    if (retval < 0) {
      fprintf (stderr, "creat: File creation error! status: %d (%s)\n",
	       retval, pathname);
      perror("Error!");
	}  
	fflush(stdout);
     
  retval = OPEN (pathname, RD_WRITE_ONLY);
    if (retval < 0) 
	  printf("Do not have the right access or this file does not exist!\n");
	retval = CHMOD(pathname, RD_WRITE_ONLY);
  retval = OPEN (pathname, RD_WRITE_ONLY);
    if (retval < 0) 
	  printf("Do not have the right access");
    printf("memsetting\n");
    memset (pathname, 0, 80);
	printf("Access Test Done!\n");

#endif // TEST_ACCESS
#ifdef TEST0_SYNC
  printf("In Test0!\n");
  for (i = 0; i < 100; i++) { // go beyond the limit
    sprintf (pathname, PATH_PREFIX "/file%d", i);
    
    retval = CREAT (pathname,RD_READ_WRITE);
    printf("Create file %d!\n", i);
    
    if (retval < 0) {
      fprintf (stderr, "creat: File creation error! status: %d (%s)\n",
	       retval, pathname);
      perror("Error!");
      
      if (i != MAX_FILES)
	exit(EXIT_FAILURE);
    }
    printf("memsetting\n");
	fflush(stdout);
    memset (pathname, 0, 80);
  }   
  retval = SYNC ();
    if (retval < 0) {
      fprintf (stderr, "creat: File creation error! status: %d (%s)\n",
	       retval, pathname);
      perror("Error!");
      
      if (i != MAX_FILES)
	exit(EXIT_FAILURE);
    }
	printf("Test 0 Done!\n");

#endif //TEST0_SYNC
#ifdef TEST0_RESTORE
  printf("In Test0!\n");
  retval = RESTORE ();
    if (retval < 0) {
      fprintf (stderr, "creat: File creation error! status: %d (%s)\n",
	       retval, pathname);
      perror("Error!");
      
      if (i != MAX_FILES)
	exit(EXIT_FAILURE);
    }
  for (i = 0; i < 100; i++) { // go beyond the limit
    sprintf (pathname, PATH_PREFIX "/file%d", i);
    
    retval = UNLINK (pathname);
    printf("Create file %d!\n", i);
    
    if (retval < 0) {
      fprintf (stderr, "creat: File creation error! status: %d (%s)\n",
	       retval, pathname);
      perror("Error!");
      
      if (i != MAX_FILES)
	exit(EXIT_FAILURE);
    }
    printf("memsetting\n");
	fflush(stdout);
    memset (pathname, 0, 80);
  }   
	printf("Test 0 Done!\n");

#endif //TEST0_RESTORE
#ifdef TEST1

  /* ****TEST 1: MAXIMUM file creation**** */

  printf("In Test1!\n");
  /* Generate MAXIMUM regular files */
  for (i = 0; i < MAX_FILES + 1; i++) { // go beyond the limit
    sprintf (pathname, PATH_PREFIX "/file%d", i);
    
    retval = CREAT (pathname,RD_READ_WRITE);
    printf("Create file %d!\n", i);
    
    if (retval < 0) {
      fprintf (stderr, "creat: File creation error! status: %d (%s)\n",
	       retval, pathname);
      perror("Error!");
      
      if (i != MAX_FILES)
	exit(EXIT_FAILURE);
    }
    printf("memsetting\n");
	fflush(stdout);
    memset (pathname, 0, 80);
  }   

  /* Delete all the files created */
  for (i = 0; i < MAX_FILES; i++) { 
    sprintf (pathname, PATH_PREFIX "/file%d", i);
	printf("ENtering unlink\n");
    retval = UNLINK (pathname);
    printf("Remove file %d!\n", i);
    
    if (retval < 0) {
      fprintf (stderr, "unlink: File deletion error! status: %d\n",
	       retval);
      
      exit(EXIT_FAILURE);
    }
    
    memset (pathname, 0, 80);
  }
  printf("Test1 Done!\n");

#endif // TEST1
  
#ifdef TEST2

  /* ****TEST 2: LARGEST file size**** */

  
  /* Generate one LARGEST file */
  retval = CREAT (PATH_PREFIX "/bigfile",RD_READ_WRITE);

  if (retval < 0) {
    fprintf (stderr, "creat: File creation error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }

  retval =  OPEN (PATH_PREFIX "/bigfile",RD_READ_WRITE); /* Open file to write to it */
  printf("Open file done!\n");
    
  
  if (retval < 0) {
    fprintf (stderr, "open: File open error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }

  fd = retval;			/* Assign valid fd */

  /* Try writing to all direct data blocks */
  retval = WRITE (fd, data1, sizeof(data1));
  printf("Write file done!\n");
  
  if (retval < 0) {
    fprintf (stderr, "write: File write STAGE1 error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }
  printf("Test2 first stage Done!\n");

#ifdef TEST_SINGLE_INDIRECT
  
  /* Try writing to all single-indirect data blocks */
  retval = WRITE (fd, data2, sizeof(data2));
  
  if (retval < 0) {
    fprintf (stderr, "write: File write STAGE2 error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }
  printf("Test2 Single Indirect Done!\n");

#ifdef TEST_DOUBLE_INDIRECT

  /* Try writing to all double-indirect data blocks */
  retval = WRITE (fd, data3, sizeof(data3));
  
  if (retval < 0) {
    fprintf (stderr, "write: File write STAGE3 error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }
  printf("Test2 Double Indirect Done!\n");

#endif // TEST_DOUBLE_INDIRECT

#endif // TEST_SINGLE_INDIRECT

#endif // TEST2

#ifdef TEST3

  /* ****TEST 3: Seek and Read file test**** */
  printf("Start lseeking\n");
  retval = LSEEK (fd, 0);	/* Go back to the beginning of your file */

  if (retval < 0) {
    fprintf (stderr, "lseek: File seek error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }

  printf("Finish Lseeking\n");
  /* Try reading from all direct data blocks */
  retval = READ (fd, addr, sizeof(data1));
  
  if (retval < 0) {
    fprintf (stderr, "read: File read STAGE1 error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }
  /* Should be all 1s here... */
  printf ("Data at addr: %s\n", addr);
  printf("Test3 1st stage Done!\n");

#ifdef TEST_SINGLE_INDIRECT

  /* Try reading from all single-indirect data blocks */
  retval = READ (fd, addr, sizeof(data2));
  
  if (retval < 0) {
    fprintf (stderr, "read: File read STAGE2 error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }
  /* Should be all 2s here... */
  printf ("Data at addr: %s\n", addr);
  printf ("Test3 Single Indirect Done!\n");

#ifdef TEST_DOUBLE_INDIRECT

  /* Try reading from all double-indirect data blocks */
  retval = READ (fd, addr, sizeof(data3));
  
  if (retval < 0) {
    fprintf (stderr, "read: File read STAGE3 error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }
  /* Should be all 3s here... */
  printf ("Data at addr: %s\n", addr);
  printf ("Test3 Double Indirect Done!\n");

#endif // TEST_DOUBLE_INDIRECT

#endif // TEST_SINGLE_INDIRECT

  /* Close the bigfile */
  retval = CLOSE(fd);
  
  if (retval < 0) {
    fprintf (stderr, "close: File close error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }

  /* Remove the biggest file */

  retval = UNLINK (PATH_PREFIX "/bigfile");
	
  if (retval < 0) {
    fprintf (stderr, "unlink: /bigfile file deletion error! status: %d\n",
	     retval);
    
    exit(EXIT_FAILURE);
  }

  printf("Test 3 done!\n");
  fflush(stdout);

#endif // TEST3

#ifdef TEST4
  
  printf("Test4 starts\n");
  /* ****TEST 4: Make directory and read directory entries**** */
  retval = MKDIR (PATH_PREFIX "/dir1");
    
  if (retval < 0) {
    fprintf (stderr, "mkdir: Directory 1 creation error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }

  retval = MKDIR (PATH_PREFIX "/dir1/dir2");
    
  if (retval < 0) {
    fprintf (stderr, "mkdir: Directory 2 creation error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }

  retval = MKDIR (PATH_PREFIX "/dir1/dir3");
    
  if (retval < 0) {
    fprintf (stderr, "mkdir: Directory 3 creation error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }

#ifdef USE_RAMDISK
  retval =  OPEN (PATH_PREFIX "/dir1",RD_READ_WRITE); /* Open directory file to read its entries */
  
  if (retval < 0) {
    fprintf (stderr, "open: Directory open error! status: %d\n",
	     retval);

    exit(EXIT_FAILURE);
  }

  fd = retval;			/* Assign valid fd */

  memset (addr, 0, sizeof(addr)); /* Clear scratchpad memory */

  while ((retval = READDIR (fd, addr))) { /* 0 indicates end-of-file */

    if (retval < 0) {
      fprintf (stderr, "readdir: Directory read error! status: %d\n",
	       retval);
      exit(EXIT_FAILURE);
    }

  //  index_node_number = atoi(&addr[14]);
    index_node_number= (addr[15]<<8)+addr[14];
    printf ("Contents at addr: [%s,%d]\n", addr, index_node_number);
  }
  printf ("Test4 Done!\n");
#endif // USE_RAMDISK
#endif // TEST4

#ifdef TEST5

  /* ****TEST 5: 2 process test**** */
  
  if((retval = fork())) {

    if(retval == -1) {
      fprintf(stderr, "Failed to fork\n");
      exit(EXIT_FAILURE);
    }

    /* Generate 300 regular files */
    for (i = 0; i < 300; i++) { 
      sprintf (pathname, PATH_PREFIX "/file_p_%d", i);
      
      retval = CREAT (pathname,RD_READ_WRITE);
      
      if (retval < 0) {
	fprintf (stderr, "(Parent) create: File creation error! status: %d\n", 
		 retval);
	exit(EXIT_FAILURE);
      }
    
      memset (pathname, 0, 80);
    }  
    
  }
  else {
    /* Generate 300 regular files */
    for (i = 0; i < 300; i++) { 
      sprintf (pathname, PATH_PREFIX "/file_c_%d", i);
      
      retval = CREAT (pathname,RD_READ_WRITE);
      
      if (retval < 0) {
	fprintf (stderr, "(Child) create: File creation error! status: %d\n", 
		 retval);

	exit(EXIT_FAILURE);
      }
      
      memset (pathname, 0, 80);
    }
  }
  printf ("Test5 Done!\n");

#endif // TEST5
  
  printf("Congratulations, you have passed all tests!!\n");
  
  return 0;
}
