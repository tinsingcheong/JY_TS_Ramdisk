#include "rw.h"
#include "ramdisk_struct.h"
#include "constant.h"
#include "ramdisk.h"

#define UL_DEBUG

#ifdef UL_DEBUG
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#endif

uint8_t* file_byte_locate(uint8_t* rd, uint16_t inodeNO, int pos){
	//return the pointer of the posth of byte in the file specified by the inodeNO
	//return NULL if not found or invalid position value
	int i;
	struct inode* file_inode;
	uint8_t* return_val;
	int size_region_type;
	int cur_direct_pointer;
	int cur_single_indirect_pointer;
	int cur_double_indirect_pointer;
#ifdef UL_DEBUG
	if(!(file_inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr, "No memory space\n");
		exit(-1);
	}
#endif
	read_inode(rd, inodeNO, file_inode);
/*	if(file_inode->type==0){
#ifdef UL_DEBUG
		printf("The file is dir file, which is unreadable\n");
#endif
		return NULL;
	}*/
	if(pos>=file_inode->size){
	//The file position pointer exceeds the file size limit
		return NULL;
	}

	/* the size of file have three regions (unit:block)
	 * [1,8]           size_region_type=0
	 * [9,72]          size_region_type=1
	 * [73, 4168]      size_region_type=2
	 */
	//determing the file size belongs to which region
	if(file_inode->size<=8*BLOCK_SIZE){
		size_region_type=0;
	}
	else if(file_inode->size>8*BLOCK_SIZE && file_inode->size<=72*BLOCK_SIZE){
		size_region_type=1;
	}
	else if(file_inode->size>72*BLOCK_SIZE && file_inode->size<=4168*BLOCK_SIZE){
		size_region_type=2;
	}
	
	int block_num=pos/BLOCK_SIZE;
	int block_offset=pos%BLOCK_SIZE;
	int indirect_block_num=(block_num-72)/64;
	int indirect_block_offset=(block_num-72)%64;
	if(size_region_type==0){
		cur_direct_pointer=file_inode->BlockPointer[block_num];
		return_val=rd+cur_direct_pointer*BLOCK_SIZE+block_offset;
	}
	else if(size_region_type==1){
		if(pos<8*BLOCK_SIZE){
			cur_direct_pointer=file_inode->BlockPointer[block_num];
			return_val=rd+cur_direct_pointer*BLOCK_SIZE+block_offset;
		}
		else{
			cur_single_indirect_pointer=file_inode->BlockPointer[8];
			cur_direct_pointer=(*((uint32_t*)(rd+cur_single_indirect_pointer*BLOCK_SIZE+(block_num-8)*4)));
			return_val=rd+cur_direct_pointer*BLOCK_SIZE+block_offset;
		}
	}
	else if(size_region_type==2){
		if(pos<8*BLOCK_SIZE){
			cur_direct_pointer=file_inode->BlockPointer[block_num];
			return_val=rd+cur_direct_pointer*BLOCK_SIZE+block_offset;
		}
		else if(pos<72*BLOCK_SIZE){
			cur_single_indirect_pointer=file_inode->BlockPointer[8];
			cur_direct_pointer=(*((uint32_t*)(rd+cur_single_indirect_pointer*BLOCK_SIZE+(block_num-8)*4)));
			return_val=rd+cur_direct_pointer*BLOCK_SIZE+block_offset;
		}
		else{
			cur_double_indirect_pointer=file_inode->BlockPointer[9];
			cur_single_indirect_pointer=(*((uint32_t*)(rd+cur_double_indirect_pointer*BLOCK_SIZE+indirect_block_num*4)));
			cur_direct_pointer=(*((uint32_t*)(rd+cur_single_indirect_pointer*BLOCK_SIZE+indirect_block_offset*4)));
			return_val=rd+cur_direct_pointer*BLOCK_SIZE+block_offset;

		}

	}

	return return_val;


	
}

uint8_t* file_byte_allocate(uint8_t* rd, uint16_t inodeNO){
	//return a pointer value for the new byte appended to the end of one file
	int i;
	struct inode* file_inode;
	uint8_t* return_val;
	int size_region_type;
	uint32_t cur_direct_pointer;
	uint32_t cur_single_indirect_pointer;
	uint32_t cur_double_indirect_pointer;
#ifdef UL_DEBUG
	if(!(file_inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr, "No memory space\n");
		exit(-1);
	}
#endif
	read_inode(rd, inodeNO, file_inode);
	int block_num=file_inode->size/BLOCK_SIZE;
	int block_offset=file_inode->size%BLOCK_SIZE;
//	printf("block_num=%d,block_offset=%d\n",block_num,block_offset);
//	fflush(stdout);
	/* the size of block num have three regions (unit:block)
	 * [0,7]           size_region_type=0
	 * 8               size_region_type=1
	 * [9,71]          size_region_type=2
	 * 72              size_region_type=3
	 * [73, 4167]      size_region_type=4
	 * 4168            size_region_type=5
	 */
	//determing the file size belongs to which region
	if(block_num<=7){
		size_region_type=0;
	}
	else if(block_num==8){
		size_region_type=1;
	}
	else if(block_num>=9 && block_num<=71){
		size_region_type=2;
	}
	else if(block_num==72){
		size_region_type=3;		
	}
	else if(block_num>=73 && block_num<=4167){
		size_region_type=4;
	}
	else{
		size_region_type=5;
	}
	if(block_offset!=0){
		//no new block needs to be allocated, just add one byte to the end of file
		//and increase the file size by one byte
		return_val=file_byte_locate(rd,inodeNO,file_inode->size-1); // acquire the pointer of the last byte 
	//	printf("current file size is %d, return_val %d\n",file_inode->size,return_val);
		return_val++;
		file_inode->size++;
	//	printf("updating\n");
		update_inode(rd,inodeNO,file_inode);
	//	printf("returning\n");
		fflush(stdout);
		return return_val;

	}
	else{
		//need to find a new block
		if(size_region_type==0){
			file_inode->BlockPointer[block_num]=find_next_free_block(rd);
			if(file_inode->BlockPointer[block_num]==-1){
#ifdef UL_DEBUG
				printf("No free block for first %dth direct blocks\n",block_num);
#endif
				return NULL;
			}
			set_bitmap(rd,file_inode->BlockPointer[block_num]);
			return_val=rd+file_inode->BlockPointer[block_num]*BLOCK_SIZE;
			file_inode->size++;
			update_inode(rd,inodeNO,file_inode);
			return return_val;
		}
		else if(size_region_type==1){
			cur_single_indirect_pointer=find_next_free_block(rd);
			if(cur_single_indirect_pointer==-1){
#ifdef UL_DEBUG
				printf("No free block for the single indirect block\n");
#endif
				return NULL;
			}
			set_bitmap(rd,cur_single_indirect_pointer);
			file_inode->BlockPointer[8]=cur_single_indirect_pointer;
			cur_direct_pointer=find_next_free_block(rd);
			if(cur_direct_pointer==-1){
#ifdef UL_DEBUG
				printf("No free block for the 0th direct block in single indirect block\n");
#endif
				return NULL;
			}
			set_bitmap(rd,cur_direct_pointer);

			*((uint32_t*)(rd+cur_single_indirect_pointer*BLOCK_SIZE))=cur_direct_pointer;
			return_val=rd+cur_direct_pointer*BLOCK_SIZE;
			file_inode->size++;
			update_inode(rd,inodeNO,file_inode);
			return return_val;
		}
		else if(size_region_type==2){
			cur_single_indirect_pointer=file_inode->BlockPointer[8];
			cur_direct_pointer=find_next_free_block(rd);
			if(cur_direct_pointer==-1){
#ifdef UL_DEBUG
				printf("No free block for the %dth direct block in single indirect block\n",block_num-8);
#endif
				return NULL;
			}
			set_bitmap(rd,cur_direct_pointer);
			*((uint32_t*)(rd+cur_single_indirect_pointer*BLOCK_SIZE+4*(block_num-8)))=cur_direct_pointer;
			return_val=rd+cur_direct_pointer*BLOCK_SIZE;
			file_inode->size++;
			update_inode(rd,inodeNO,file_inode);
			return return_val;
		}
		else if(size_region_type==3){
			cur_double_indirect_pointer=find_next_free_block(rd);
			if(cur_double_indirect_pointer==-1){
#ifdef UL_DEBUG
				printf("No free block for the double indirect block\n");
#endif
				return NULL;
			}
			file_inode->BlockPointer[9]=cur_double_indirect_pointer;
			set_bitmap(rd,cur_double_indirect_pointer);
			cur_single_indirect_pointer=find_next_free_block(rd);
			if(cur_single_indirect_pointer==-1){
#ifdef UL_DEBUG
				printf("No free block for the 1st single indirect block in the double indirect block\n");
#endif
				return NULL;
			}
			set_bitmap(rd,cur_single_indirect_pointer);
			*((uint32_t*)(rd+cur_double_indirect_pointer*BLOCK_SIZE))=cur_single_indirect_pointer;
			cur_direct_pointer=find_next_free_block(rd);
			if(cur_direct_pointer==-1){
#ifdef UL_DEBUG
				printf("No free block for the 1st direct block in the 1st single indirect block in the double indirect block\n");
#endif
				return NULL;
			}
			set_bitmap(rd,cur_direct_pointer);
			*((uint32_t*)(rd+cur_single_indirect_pointer*BLOCK_SIZE))=cur_direct_pointer;
			return_val=rd+cur_direct_pointer*BLOCK_SIZE;
			file_inode->size++;
			update_inode(rd,inodeNO,file_inode);
			return return_val;
		}
		else if(size_region_type==4){
			cur_double_indirect_pointer=file_inode->BlockPointer[9];
			if((block_num-72)%64==0){
				cur_single_indirect_pointer=find_next_free_block(rd);
				if(cur_single_indirect_pointer==-1){
#ifdef UL_DEBUG
					printf("No free block for the %dth single indirect block in the double indirect block\n",(block_num-72)/64);
#endif
					return NULL;
				}
				set_bitmap(rd,cur_single_indirect_pointer);
				*((uint32_t*)(rd+cur_double_indirect_pointer*BLOCK_SIZE+4*((block_num-72)/64)))=cur_single_indirect_pointer;

				cur_direct_pointer=find_next_free_block(rd);
				if(cur_direct_pointer==-1){
#ifdef UL_DEBUG
					printf("No free block for the 0th direct block in the %dth single indirect block in the double indirect block\n",
								(block_num-72)/64);
#endif
					return NULL;
				}
				set_bitmap(rd,cur_direct_pointer);
				*((uint32_t*)(rd+cur_single_indirect_pointer*BLOCK_SIZE))=cur_direct_pointer;
				return_val=rd+cur_direct_pointer*BLOCK_SIZE;
				file_inode->size++;
				update_inode(rd,inodeNO,file_inode);
				return return_val;
			}
			else{
				cur_single_indirect_pointer=(*((uint32_t*)(rd+cur_double_indirect_pointer*BLOCK_SIZE+4*((block_num-72)/64))));
				cur_direct_pointer=find_next_free_block(rd);
				if(cur_direct_pointer==-1){
#ifdef UL_DEBUG
					printf("No free block for the %dth direct block in the %dth single indirect block in the double indirect block\n",
								(block_num-72)%64,(block_num-72)/64);
#endif
					return NULL;
				}
				set_bitmap(rd,cur_direct_pointer);
				*((uint32_t*)(rd+cur_single_indirect_pointer*BLOCK_SIZE+4*((block_num-72)%64)))=cur_direct_pointer;
				return_val=rd+cur_direct_pointer*BLOCK_SIZE;
				file_inode->size++;
				update_inode(rd,inodeNO,file_inode);
				return return_val;
			}
		}

		else{
#ifdef UL_DEBUG
			printf("The largest possible of one file is reached\n");
#endif

			return NULL;


		}


		
	}

	
	
	
}

int read_ramdisk(uint8_t* rd, uint16_t inodeNO, int pos, uint8_t* buf, int length){
	
	int i=0;
	int count=0;
	struct inode* file_inode;
	uint8_t* ptr;
#ifdef UL_DEBUG
	if(!(file_inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr, "No memory space\n");
		exit(-1);
	}
#endif
	read_inode(rd,inodeNO,file_inode);
	if(file_inode->type==0){
#ifdef UL_DEBUG
		printf("The file is dir file, which is unreadable\n");
#endif
		return -1;
	}
	for(i=0;i<length;i++){
		ptr=file_byte_locate(rd, inodeNO, pos+i);
		if(ptr==NULL)
			break;
		*(buf+i)=*ptr;
		count++;
	}

	return count;

	
}

int write_ramdisk(uint8_t* rd, uint16_t inodeNO, int pos, uint8_t* buf, int length){
	
	int i=0;
	int count=0;
	struct inode* file_inode;
	uint8_t* ptr;
#ifdef UL_DEBUG

	if(!(file_inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr, "No memory space\n");
		exit(-1);
	}
#endif
	read_inode(rd,inodeNO,file_inode);
	if(file_inode->type==0){
#ifdef UL_DEBUG
		printf("The file is dir file, which is unwritable\n");
#endif
		return -1;
	}

	if(pos>file_inode->size){
#ifdef UL_DEBUG
		printf("The start pos is out of file size range\n");
#endif
		return -1;
	}

	for(i=0;i<length;i++){
	//	printf("pos+i=%d,file_inode->size=%d\n",pos+i,file_inode->size);
		if(pos+i==file_inode->size){
			ptr=file_byte_allocate(rd, inodeNO);
		}
		else{
			ptr=file_byte_locate(rd, inodeNO, pos+i);
		}
		if(ptr==NULL)
			break;
	//	printf("Copying from %x:%c to ptr %x\n", buf+i, *(buf+i), ptr);
	//	fflush(stdout);
		*ptr=*(buf+i);
	//	printf("Copy finished\n");
	//	fflush(stdout);
		count++;
	//	printf("count=%d\n",count);
		fflush(stdout);
		read_inode(rd,inodeNO,file_inode);

	}

	partial_update_superblock(rd);

	return count;


}

int readdir(uint8_t* rd, uint16_t inodeNO, int pos, uint8_t* buf){
	int i=0;
	int count=0;
	struct inode* file_inode;
	uint8_t* ptr;
#ifdef UL_DEBUG
	if(!(file_inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr, "No memory space\n");
		exit(-1);
	}
#endif
	read_inode(rd,inodeNO,file_inode);
	if(file_inode->type==1){
#ifdef UL_DEBUG
		printf("The file is regular file, which is unreadable for readdir\n");
#endif
		return -1;
	}

	for(i=0;i<16;i++){
		ptr=file_byte_locate(rd, inodeNO, pos*16+i);
		if(ptr==NULL)
			break;
		*(buf+i)=*ptr;
		count++;
	}

	if(count==0){
		return 0;
	}
	else if(count==16){
		return 1;
	}
	else{
		return -1;
	}


}

