#include "ramdisk.h"
#include "rw.h"
#include "file_func.h"

#ifdef UL_DEBUG
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define FILE_NUM 10
int main(int argc, char* argv[]){
	int i=0;
	uint8_t* rd;
    uint8_t buf[30]="adfasdfasgfaaaaaaaaaaa";
    
	char name[14];
	char search_name[14];
	rd=ramdisk_init();
	int root_block_id=search_file(rd, "/");
    int search_file_inodeNO;
    int remove_flag;
    int a;
	printf("root block id is %d\n", root_block_id);
	
	struct inode* root_inode;
	if(!(root_inode=(struct inode*)malloc(sizeof(struct inode)))){
		printf("No mem space!\n");
		exit(-1);
	}
    //create_dir(rd,0,"ts_dir");
	//create a file under root named Jiayi.txt
    create_file(rd, 0, "ts_file.txt");
    for (i = 0; i<1024 ; i++)
    {
        a = write_ramdisk(rd,1,i*20,buf,20);
	    printf("a = %d; i = %d\n", a, i);
//        sprintf(name, "ts_%d.txt", i);
//        create_file(rd, 1, name);
    }

 /*   for (i = 1; i<=FILE_NUM ; i++)
    {
        sprintf(search_name, "/ts_%d.txt", i);
	    search_file_inodeNO = search_file(rd,search_name);
        printf("File InodeNO is:%d\n", search_file_inodeNO);
    }*/
    //remove_flag=remove_file(rd,0,2,"ts_2.txt");
    //(remove_flag==(-1))?(printf("Remove fail!\n")):(printf("Remove success!\n"));
    //search_file_inodeNO=search_file(rd,"/ts_2.txt");
    //printf("File InodeNO is:%d\n", search_file_inodeNO);
    remove_flag=remove_file(rd,0,1,"ts_file.txt");
    (remove_flag==(-1))?(printf("Remove fail!\n")):(printf("Remove success!\n"));
    search_file_inodeNO=search_file(rd,"/ts_file.txt");
    printf("File InodeNO is:%d\n", search_file_inodeNO);
    
	return 0;
}

#endif
