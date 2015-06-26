#define BUFF 4096
#define NIN 12288
#define NDB 12288
#define FIN 66
#define FSB 4030
#define DIR 1
#define REGF 2

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<malloc.h>
#include<stdlib.h>
#include<string.h>

typedef struct superblock{
  unsigned int fs_size;

  unsigned int total_num_of_inodes;
  unsigned int no_of_free_inodes;
  unsigned int next_free_inode_index;
  unsigned int next_free_inode_offset;
  unsigned int inodeblock_index;

  unsigned int total_num_of_datablocks;
  unsigned int no_of_free_datablocks;
  unsigned int next_free_datablock_index;
  unsigned int next_free_datablock_offset;
  unsigned int datablock_index;

  unsigned int root_inode_index;
  unsigned int root_db_index;
  unsigned int wd_inode_offset;
  unsigned int wd_inode_index;
  unsigned int wd_db_index;
  short int flag;

  char futureuse[FSB];

  char i_bmap[NIN];
  char db_bmap[NDB];
}SuperBlock;

typedef struct filedata{
  unsigned int blocks[10];
}FileData;

typedef struct diskinode{
  unsigned int inum;
  short int type;
  unsigned int num_of_blocks;
  unsigned int size;
  FileData fd;
  FileData *ifd;
  char futureuse[FIN];
}DiskInode;

typedef struct datablock{
  char data[BUFF];
}DataBlock;

typedef struct dir{
  unsigned int inum;
  char fname[16];
}MyDir;

int myopen(char *);
int myread(int, void *, int);
void mywrite(int, void *, int);
void initfilesystem(unsigned int, char *);
SuperBlock * initsuperblock(unsigned int);
DiskInode * initdiskinodeblock();
DataBlock * initdatablock();
void writesuperblock(SuperBlock *, int);
void writedatablock(SuperBlock *, DataBlock *, int);
void writedir(SuperBlock *sb, DiskInode *, int, MyDir *);
SuperBlock * readsuperblock(int);
void printsuperblock(SuperBlock *);
void printinodeblock(DiskInode *);
void printdir(DiskInode *, MyDir *);
unsigned int dbget(SuperBlock *, int);
DiskInode *ialloc(SuperBlock *, int);
DiskInode *iget(SuperBlock *, int, int);
void iput(SuperBlock *, DiskInode *, int);
void ifree(SuperBlock *, DiskInode *, int);
DiskInode *namei(SuperBlock *, DiskInode *, int, char*);
MyDir *dirget(SuperBlock *, int, int);
void mymkdir(SuperBlock *, DiskInode *, MyDir *, int, char *);
int openfile(SuperBlock *, DiskInode *, MyDir *, int, char *, char *);
void myfwrite(SuperBlock *, DiskInode *, int, char **);
void mymkfile(SuperBlock *, DiskInode *, DiskInode *, MyDir *, int, char *);
int myrmdir(SuperBlock *,DiskInode *,MyDir *, int,char *);
