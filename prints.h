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

void printsuperblock(SuperBlock *sb){
  int i;

  printf("sb->fs_size = %u\n", sb->fs_size);
  printf("sb->total_num_of_inodes = %u\n", sb->total_num_of_inodes);
  printf("sb->no_of_free_inodes = %u\n", sb->no_of_free_inodes);
  printf("sb->next_free_inode_index = %u\n", sb->next_free_inode_index);
  printf("sb->next_free_inode_offset = %u\n", sb->next_free_inode_offset);
  printf("sb->inodeblock_index = %u\n", sb->inodeblock_index);
  printf("sb->total_num_of_datablocks = %u\n", sb->total_num_of_datablocks);
  printf("sb->no_of_free_datablocks = %u\n", sb->no_of_free_datablocks);
  printf("sb->next_free_datablock_index = %u\n", sb->next_free_datablock_index);
  printf("sb->next_free_datablock_offset = %u\n", sb->next_free_datablock_offset);
  printf("sb->datablock_index = %u\n", sb->datablock_index);
  printf("sb->root_inode_index = %u\n", sb->root_inode_index);
  printf("sb->root_db_index = %u\n", sb->root_db_index);
  printf("sb->wd_inode_offset = %u\n", sb->wd_inode_offset);
  printf("sb->wd_inode_index = %u\n", sb->wd_inode_index);
  printf("sb->wd_db_index = %u\n", sb->wd_db_index);
  printf("sb->flag = %d\n", sb->flag);
  printf("sb->i_bmap = ");
  for(i=0; i<10; i++)
    printf("%d", sb->i_bmap[i]);
  printf("\n");
  printf("sb->db_bmap = ");
  for(i=0; i<10; i++)
    printf("%d", sb->db_bmap[i]);
  printf("\n");
}

void printinodeblock(DiskInode *di){
  printf("di->inum = %u\n", di->inum);
  printf("di->type = %d\n", di->type);
  printf("di->num_of_blocks = %u\n", di->num_of_blocks);
  printf("di->size = %u\n", di->size);
  printf("di->fd.blocks[0] = %u\n", di->fd.blocks[0]);
}

void printdir(DiskInode *di, MyDir *dir){
  int i;

  for(i=0; i<di->size; i+=sizeof(MyDir))
    printf("dir->inum = %u\t dir->fname = %s\n",(dir+i)->inum, (dir+i)->fname);
}
