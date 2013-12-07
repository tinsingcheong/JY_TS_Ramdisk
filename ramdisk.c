#include "ramdisk_struct.h"
#include "constant.h"
#define UL_DEBUG //for user level debugging

#ifdef ULDEBUG
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#endif

void update_superblock(uint8_t* rd, struct super_block* SuperBlock){
	int i;
	rd[SUPERBLOCK_BASE]=(uint8_t)(SuperBlock->FreeBlockNum & 0x00ff);
	rd[SUPERBLOCK_BASE+1]=(uint8_t)(SuperBlock->FreeBlockNum>>BYTELEN);
	rd[SUPERBLOCK_BASE+2]=(uint8_t)(SuperBlock->FreeInodeNum & 0x00ff);
	rd[SUPERBLOCK_BASE+3]=(uint8_t)(SuperBlock->FreeInodeNum>>BYTELEN);
	for(i=0;i<INODEBITMAP_SIZE;i++){
		rd[INODEBITMAP_BASE+i]=SuperBlock->InodeBitmap[i];
	}
}

void read_superblock(uint8_t* rd, struct superblock* SuperBlock){
	int i;
	SuperBlock->FreeBlockNum=(((uint16_t)rd[SUPERBLOCK_BASE+1])<<BYTELEN) | (uint16_t)rd[SUPERBLOCK_BASE];
	SuperBLock->FreeInodeNum=(((uint16_t)rd[SUPERBLOCK_BASE+3])<<BYTELEN) | (uint16_t)rd[SUPERBLOCK_BASE+2];
	for(i=0;i<INODEBITMAP_SIZE;i++){
		SuperBlock->InodeBitmap[i]=rd[INODEBITMAP_BASE+i];
	}
}

void update_inode(uint8_t* rd, int NodeNO, struct inode* Inode){
	int i;
	rd[INODE_BASE+NodeNO*64]=Inode->type;
	rd[INODE_BASE+NodeNO*64+1]=(uint8_t)(Inode->size & 0x000000ff);
	rd[INODE_BASE+NodeNO*64+2]=(uint8_t)((Inode->size & 0x0000ff00)>>BYTELEN);
	rd[INODE_BASE+NodeNO*64+3]=(uint8_t)((Inode->size & 0x00ff0000)>>(2*BYTELEN));
	rd[INODE_BASE+NodeNO*64+4]=(uint8_t)(Inode->size>>(3*BYTELEN));
	for(i=0;i<10;i++){
		rd[INODE_BASE+NodeNO*64+5+i*4]=(uint8_t)(Inode->BlockPointer[i] & 0x000000ff);
		rd[INODE_BASE+NodeNO*64+6+i*4]=(uint8_t)((Inode->BlockPointer[i] & 0x0000ff00)>>BYTELEN);
		rd[INODE_BASE+NodeNO*64+7+i*4]=(uint8_t)((Inode->BlockPointer[i] & 0x00ff0000)>>(2*BYTELEN));
		rd[INODE_BASE+NodeNO*64+8+i*4]=(uint8_t)(Inode->BlockPointer[i]>>(3*BYTELEN));
	}
}

void read_inode(uint8_t* rd, int NodeNO, struct inode* Inode){
	Inode->type=rd[INODE_BASE+NodeNO*64];
	Inode->size=(uint32_t)(rd[INODE_BASE+NodeNO*64+1]) | ((uint32_t)(rd[INODE_BASE+NodeNO*64+2])<<BYTELEN) |
				((uint32_t)(rd[INODE_BASE+NodeNO*64+3])<<(2*BYTELEN)) | ((uint32_t)(rd[INODE_BASE+NodeNO*64+4])<<(3*BYTELEN));
	for(i=0;i<10;i++){
		Inode->BlockPointer[i]=(uint32_t)(rd[INODE_BASE+NodeNO*64+5+i*4]) | ((uint32_t)(rd[INODE_BASE+NodeNO*64+6+i*4])<<BYTELEN) |
				((uint32_t)(rd[INODE_BASE+NodeNO*64+7+i*4])<<(2*BYTELEN)) | ((uint32_t)(rd[INODE_BASE+NodeNO*64+8+i*4])<<(3*BYTELEN));
	}
}

void set_bitmap(uint8_t* rd, int BlockNO){
	int byte_location=BlockNO/BYTELEN;
	uint8_t set=1;
	set=set<<(BlockNO & 0x00000007);//last three bits to address the bit pos in one byte
	rd[BITMAP_BASE+byte_location]|=set;	
}

void clr_bitmap(uint8_t*rd, int BlockNO){
	int byte_location=BlockNO/BYTELEN;
	uint8_t clr=1;
	clr=clr<<(BlockNO & 0x00000007);
	rd[BITMAP_BASE+byte_location]&=(~clr);	

}

int find_next_free_block(uint8_t* rd){
	int i,j;
	uint8_t tmp;
	for(i=0;i<BITMAP_SIZE;i++){
		tmp=rd[BITMAP_BASE+i];
		for(j=0;j<BYTELEN;j++){
			if(tmp&0x01==0)
				return(i*BYTELEN+j);
			tmp=tmp>>1;
		}
	}
	return(-1); //-1 means no empty block
}

int bitmap_sum_up(uint8_t* rd){
	int count=0;
	int i,j;
	for(i=0;i<BITMAP_SIZE;i++){
		tmp=rd[BITMAP_BASE+i];
		for(j=0;j<BYTELEN;j++){
			if(tmp&0x01)
				count++;
			tmp=tmp>>1;
		}
	}
	return count;
}

void set_inode_bitmap(uint8_t* rd, int InodeNO){
	int byte_location=InodeNO/BYTELEN;
	uint8_t set=1;
	set=set<<(InodeNO & 0x00000007);
	rd[INODEBITMAP_BASE+byte_location]|=set;
}

void clr_inode_bitmap(uint8_t* rd, int InodeNO){
	int byte_location=InodeNO/BYTELEN;
	uint8_t clr=1;
	clr=clr<<(InodeNO & 0x00000007);cd 
	rd[INODEBITMAP_BASE+byte_location]&=(~clr);
}

int inode_bitmap_sum_up(uint8_t* rd){
	int count=0;
	int i,j;
	for(i=0;i<INODEBITMAP_SIZE;i++){
		tmp=rd[INODEBITMAP_BASE+i];
		for(j=0;j<BYTELEN;j++){
			if(tmp&0x01)
				count++;
			tmp=tmp>>1;
		}
	}
	return count;
}




uint8_t* ramdisk_init(){
	int i;
	uint8_t* ramdisk;
#ifdef UL_DEBUG
	if(!(ramdisk=(uint8_t*)malloc(RAMDISK_SIZE*sizeof(uint8_t)))){
		fprintf(stderr,"No sufficient mem space for ramdisk!\n");
		exit(-1);
	}
#endif
	//Nullify all the data in ramdisk
	memset(ramdisk,0,RAMDISK_SIZE);

	//Init the bitmap
	for(i=0;i<=(BITMAP_LIMIT+1)/BLOCK_SIZE;i++){
		set_bitmap(ramdisk,i);
	}

	//Init the root directory
	int root_bid=find_next_free_block(ramdisk);//BlockNO for root dir
	struct inode* root_inode;
#ifdef UL_DEBUG
	if(!(root_inode)=(struct inode*)malloc(sizeof(struct inode))){
		fprintf(stderr, "No sufficient mem space for root dir!\n");
		exit(-1);
	}
#endif
	root_inode->type=0;
	root_inode->size=BLOCK_SIZE;
	root_inode->BlockPointer[0]=root_bid;
	updata_inode(ramdisk,0,root_inode);
	
	//Init the superblock
	struct super_block* InitSuperBlock;
#ifdef UL_DEBUG
	if(!(InitSuperBlock=(struct super_block*)malloc(sizeof(struct super_block)))){
		fprintf(stderr,"No sufficient mem\n");
		exit(-1);
	}
#endif 
	InitSuperBlock->FreeBlockNum=BLOCK_NUM-(BITMAP_LIMIT+1)/BLOCK_SIZE-1;//The root takes an additional block besides inodes, superblock and bitmap
	InitSuperBlock->FreeInodeNum=INODE_NUM-1;//The root dir takes one inode
	memset(InitSuperBlock->InodeBitmap,0,INODEBITMAP_SIZE);
	update_superblock(ramdisk,InitSuperBlock);
	
	//Init the inode bitmap in superblock
	set_inode_bitmap(ramdisk,0);


	return ramdisk;	

}

int search_file(uint8_t* rd, char* path){
	int i,j;
	struct path* path_list;
	struct path* path_root;
	struct path* path_leave;
	char tmp[14];
	int inodeNO;
//parse the path by token '/'

#ifdef UL_DEBUG
	if(!(path_list=(struct path*)malloc(sizeof(struct path)))){
		fprintf(stderr, "No space for path list struct\n");
		exit(-1);
	}
#endif
	path_list->next=NULL;

	if(path[0]!='/'){//the first character of path should be root path
		return -1;
	}
	strcpy(path_list->filename,"/");//the root filename is "/"
	path_root=path_list;
	path_leave=path_list;

	i=1;
	j=0;
	while(path[i]){//if not '\0'
		if(path[i]=='/'){
			if(j==0){
#ifdef UL_DEBUG
				printf("Error, Wrong path\n");
#endif
				return -1;
			}
			else if(j>=13){
#ifdef UL_DEBUG
				printf("Error, dir name %s too long\n",tmp);
#endif
				return -1;
			}
			else{
				tmp[j]='\0';
				j=0;
#ifdef UL_DEBUG
				if(!(path_list=(struct path*)malloc(sizeof(struct path)))){
					fprintf(stderr, "No space for path list struct\n");
					exit(-1);
				}
#endif
				//add the path_list to the end of the link list
				path_list->next=NULL;
				path_leave->next=path_list;
				path_leave=path_list;
				strcpy(path_leave->filename,tmp);

			}
			j=0;
		}
		else{
			tmp[j]=path[i];
			j++;
			
		}
		i++;
	}
	if(j==0&&i>1){
#ifdef UL_DEBUG
		printf("Error, Wrong path\n");
#endif
		return -1;
	}
	else if(j==0&&i==1){
		//means the path is "/", return the 0th inode
		return 0
	}
	else if(j>=13){
#ifdef UL_DEBUG
		printf("Error, dir name %s too long\n",tmp);
#endif
		return -1;
	}
	else{
		tmp[j]='\0';
#ifdef UL_DEBUG
		if(!(path_list=(struct path*)malloc(sizeof(struct path)))){
			fprintf(stderr, "No space for path list struct\n");
			exit(-1);
		}
#endif
		//add the path_list to the end of the link list
		path_list->next=NULL;
		path_leave->next=path_list;
		path_leave=path_list;

		strcpy(path_leave->filename,tmp);
	}

// the path list is started with path_root and ended with path_leave
//
	int current_inodeid=0;//start from the root inode
	struct inode* current_inode;
	uint32_t current_blockid;
	int size_region_type;
#ifdef UL_DEBUG
	if(!(current_inode=(struct inode*)malloc(sizeof(struct inode)))){
		fprintf(stderr,"No mem space!\n");
		exit(-1);
	}
#endif

	for(path_list=path_root->next;path_list!=NULL;path_list=path_list->next){
		read_inode(rd, current_inodeid, current_inode);
		if(current_inode->type==1 && path_list->next!=NULL){
#ifdef UL_DEBUG
			printf("The dir is actually a regular file, wrong path\n");
#endif
			return -1;
		}
		/* the size of file have three regions (unit:block)
		 * [1,8]           size_region_type=0
		 * [9,72]          size_region_type=1
		 * [73, 1067008]   size_region_type=2
		 */
		//first traverse the 8 direct block pointers
		//determing the file size belongs to which region
		if(current_inode->size<=8*BLOCKSIZE){
			size_region_type=0;
		}
		else if(current_inode->size>8*BLOCKSIZE && current_inode->size<=72*BLOCKSIZE){
			size_region_type=1;
		}
		else if(current_inode->size>72*BLOCKSIZE && current_inode->size<=4168*BLOCKSIZE){
			size_region_type=2;
		}



		
		for(i=0;i<8;i++){
			current_blockid=current_inodeid->BlockPointer[i];
			for(j=0;j<128;j+=2){//128 
			}
		}
	}

}
	

