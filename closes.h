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

int myclose(int fd){
  return(close(fd));
}

void ifree(SuperBlock *sb, DiskInode *di, int fd){
  int i, j, offset;
  DataBlock *db = initdatablock();

  sb->no_of_free_inodes++;
  sb->i_bmap[di->inum] = '\0';
  for(i=0; i<di->num_of_blocks; i++){
    sb->db_bmap[di->fd.blocks[i]] = '\0';
    sb->no_of_free_datablocks++;
    offset = lseek(fd, (sb->datablock_index+sizeof(DataBlock)*di->fd.blocks[i]), SEEK_SET);
    if(offset == -1)
      printf("Error: Unable to seek file pointer!!!\n");
    writedatablock(sb, db, fd);
  }
  offset = lseek(fd, (sb->inodeblock_index+sizeof(DiskInode)*di->inum), SEEK_SET);
  if(offset == -1)
    printf("Error: Unable to seek file pointer!!!\n");
  di = initdiskinodeblock();
  iput(sb, di, fd);

  sb->flag = 1;
}

