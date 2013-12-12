#include"ramdisk.h"
#include"file_func.h"

#ifdef UL_DEBUG
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define FILE_NUM 20
int main(int argc, char* argv[]){
	int i=0;
	uint8_t* rd;
	char name[14];
	char search_name[14];
	rd=ramdisk_init();
	int root_block_id=search_file(rd, "/");
    int search_file_inodeNO;

	printf("root block id is %d\n", root_block_id);
	
	struct inode* root_inode;
	if(!(root_inode=(struct inode*)malloc(sizeof(struct inode)))){
		printf("No mem space!\n");
		exit(-1);
	}
	//create a file under root named Jiayi.txt
    for (i = 1; i<=FILE_NUM ; i++)
    {
        sprintf(name, "ts_file%d.txt", i);
        create_file(rd, 0, name);
    }
    for (i = 1; i<=FILE_NUM ; i++)
    {
        sprintf(search_name, "/ts_file%d.txt", i);
	    search_file_inodeNO = search_file(rd,search_name);
        printf("File InodeNO is:%d\n", search_file_inodeNO);
    }
	return 0;
}

#endif
