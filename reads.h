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


int myread(int fd, void *buf, int n){
  return (read(fd, buf, n));
}

SuperBlock * readsuperblock(int fd){
  int i, offset;
  SuperBlock *sb = (SuperBlock *)malloc(sizeof(SuperBlock));
  offset = lseek(fd, 0, SEEK_SET);
  if(offset == -1)
    printf("Error: Unable to seek file pointer!!!\n");

  myread(fd,&(sb->fs_size),sizeof(sb->fs_size));
  myread(fd,&(sb->total_num_of_inodes),sizeof(sb->total_num_of_inodes));
  myread(fd,&(sb->no_of_free_inodes),sizeof(sb->no_of_free_inodes));
  myread(fd,&(sb->next_free_inode_index),sizeof(sb->next_free_inode_index));
  myread(fd,&(sb->next_free_inode_offset),sizeof(sb->next_free_inode_offset));
  myread(fd,&(sb->inodeblock_index),sizeof(sb->inodeblock_index));
  myread(fd,&(sb->total_num_of_datablocks),sizeof(sb->total_num_of_datablocks));
  myread(fd,&(sb->no_of_free_datablocks),sizeof(sb->no_of_free_datablocks));
  myread(fd,&(sb->next_free_datablock_index),sizeof(sb->next_free_datablock_index));
  myread(fd,&(sb->next_free_datablock_offset),sizeof(sb->next_free_datablock_offset));
  myread(fd,&(sb->datablock_index),sizeof(sb->datablock_index));
  myread(fd,&(sb->root_inode_index),sizeof(sb->root_inode_index));
  myread(fd,&(sb->root_db_index),sizeof(sb->root_db_index));
  myread(fd,&(sb->wd_inode_offset),sizeof(sb->wd_inode_offset));
  myread(fd,&(sb->wd_inode_index),sizeof(sb->wd_inode_index));
  myread(fd,&(sb->wd_db_index),sizeof(sb->wd_db_index));
  myread(fd,&(sb->flag),sizeof(sb->flag));
  for(i=0; i<FSB; i++)
    myread(fd,&(sb->futureuse[i]),sizeof(sb->futureuse[i]));
  for(i=0; i<NIN; i++)
    myread(fd,&(sb->i_bmap[i]),sizeof(sb->i_bmap[i]));
  for(i=0; i<NDB; i++)
    myread(fd,&(sb->db_bmap[i]),sizeof(sb->db_bmap[i]));

  return sb;
}

