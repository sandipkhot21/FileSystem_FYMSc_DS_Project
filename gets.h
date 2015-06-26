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

DiskInode *iget(SuperBlock *sb, int fd, int inum){
  int i, offset;
  DiskInode *di = NULL;

  if(sb->i_bmap[inum] == '\0'){
    printf("Error: Empty Inode!!!\n");
    return di;
  }

  offset = lseek(fd, sb->inodeblock_index+inum*sizeof(DiskInode), SEEK_SET);
  if(offset == -1){
    printf("Error: Unable to seek file pointer!!!\n");
    return di;
  }
  di = initdiskinodeblock();

  myread(fd, &(di->inum), sizeof(di->inum));
  myread(fd, &(di->type), sizeof(di->type));
  myread(fd, &(di->num_of_blocks), sizeof(di->num_of_blocks));
  myread(fd, &(di->size), sizeof(di->size));
  for(i=0; i<10; i++)
    myread(fd, &(di->fd.blocks[i]), sizeof(di->fd.blocks[i]));
  myread(fd, di->ifd, sizeof(di->ifd));
  for(i=0; i<FIN; i++)
    myread(fd, &(di->futureuse[i]), sizeof(di->futureuse[i]));

  return di;
}

MyDir *dirget(SuperBlock *sb, int fd, int inum){
  int i, j, offset;
  DiskInode *di = NULL;
  MyDir *dir = NULL;

  di = iget(sb, fd, inum);
  dir = (MyDir*)malloc(di->size);

  offset = lseek(fd, di->fd.blocks[0], SEEK_SET);
  if(offset == -1)
    printf("Error: Unable to seek file pointer!!!\n");

  for(j=0; j<=di->size; j+=sizeof(MyDir)){
    myread(fd, &((dir+j)->inum), sizeof((dir+j)->inum));
    myread(fd, &((dir+j)->fname), sizeof((dir+j)->fname));
  }

  return dir;
}

char **fget(SuperBlock *sb, DiskInode *fi, int fd){
  int i, j, offset;
  char *l, **buf;
  buf = (char **)malloc(10);

  for(i=0; i<fi->num_of_blocks; i++){
    offset = lseek(fd, fi->fd.blocks[i], SEEK_SET);
    if(offset == -1)
      printf("Error: Unable to seek file pointer!!!\n");
    l = (char*)malloc(BUFF);
    j = myread(fd,l,BUFF);
    printf("\t%d\n", j);
    buf[i]=l;
  }
  return buf;
}

unsigned int dbget(SuperBlock *sb, int fd){
  unsigned int db_address;

  if(sb->no_of_free_datablocks == 0){
    printf("Error: No more free datablocks available!!!\n");
    return 0;
  }
  db_address = sb->next_free_datablock_index;

  sb->db_bmap[sb->next_free_datablock_offset] = '1';
  while(sb->db_bmap[sb->next_free_datablock_offset] != '\0'){
    if(sb->next_free_datablock_offset++ == NDB)
      sb->next_free_datablock_offset = 1;
  }
  sb->no_of_free_datablocks--;
  sb->next_free_datablock_index = sb->datablock_index+sizeof(DataBlock)*sb->next_free_datablock_offset;

  sb->flag = 1;
  return db_address;
}
