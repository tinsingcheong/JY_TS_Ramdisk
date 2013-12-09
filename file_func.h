#ifndef FILE_FUNC_H
#define FILE_FUNC_H
int write_file (uint8_t* rd, int InodeNO, int pos, char* string, int length);
int read_file (uint8_t* rd, int InodeNO, int pos);
int read_dir (uint8_t* rd, int InodeNO);
int get_file_size (uint8_t* rd, int InodeNO);
int create_file (uint8_t* rd, int ParentInodeNO, char* name);
int create_dir (uint8_t* rd, int ParentDirInode, char* name);
int remove_file (uint8_t* rd, int ParentInodeNO, int InodeNO, char* name);
int remove_dir (uint8_t* rd, int ParentInodeNO, int InodeNO, char* name);
void switch_block (uint8_t* rd, int DirInodeNO, char);
#endif
