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


void initfilesystem(unsigned int fs_size, char *file){
  int i, offset;
  int fd = myopen(file);
  SuperBlock *sb = (SuperBlock*)malloc(sizeof(SuperBlock));
  DiskInode *di = initdiskinodeblock();
  DataBlock *db = initdatablock();
  MyDir *dir = (MyDir*)malloc(sizeof(MyDir)*2);

  //initialize superblock
  sb->fs_size = fs_size;
  sb->total_num_of_inodes = NIN;
  sb->no_of_free_inodes = NIN-2;
  sb->next_free_inode_index = sizeof(SuperBlock)+2*sizeof(DiskInode);
  sb->next_free_inode_offset = 2;
  sb->inodeblock_index = sizeof(SuperBlock);

  sb->total_num_of_datablocks = NDB;
  sb->no_of_free_datablocks = NDB-2;
  sb->next_free_datablock_index = sizeof(SuperBlock)+sb->total_num_of_inodes*sizeof(DiskInode)+2*sizeof(DataBlock);
  sb->next_free_datablock_offset = 2;
  sb->datablock_index = sizeof(SuperBlock)+sb->total_num_of_inodes*sizeof(DiskInode);

  sb->root_inode_index = sizeof(SuperBlock)+sizeof(DiskInode);
  sb->root_db_index = sizeof(SuperBlock)+sb->total_num_of_inodes*sizeof(DiskInode)+sizeof(DataBlock);
  sb->wd_inode_offset = 1;
  sb->wd_inode_index = sb->root_inode_index;
  sb->wd_db_index = sb->root_db_index;
  sb->flag = 0;

  for(i=0; i<FSB; i++)
    sb->futureuse[i] = '\0';
  for(i=0; i<NIN; i++)
    sb->i_bmap[i] = '\0';
  for(i=0; i<NDB; i++)
    sb->db_bmap[i] = '\0';
  sb->i_bmap[1] = '1';
  sb->db_bmap[1] = '1';

  writesuperblock(sb,fd);
  //blankout rest of the inode
  for(i=0; i<NIN; i++){
    di->inum = i;
    iput(sb, di, fd);
  }
  for(i=0; i<NDB; i++)
    writedatablock(sb, db,fd);

  //initialize and write root inode
  di->inum = 1;
  di->type = DIR;
  di->num_of_blocks = 1;
  di->size = sizeof(MyDir)*2;
  di->fd.blocks[0] = sb->root_db_index;
  for(i=1; i<10; i++)
    di->fd.blocks[i] = 0;
  di->ifd = NULL;
  for(i=0; i<FIN; i++)
    di->futureuse[i] = '\0';
  iput(sb,di,fd);

  //Initialise root dir
  dir->inum = 1;
  strcpy(dir->fname,".");
  (dir+sizeof(MyDir))->inum = 1;
  strcpy((dir+sizeof(MyDir))->fname, "..");
  writedir(sb,di,fd,dir);

  myclose(fd);
  free(sb);
  free(di);
  free(db);
  free(dir);
}

DiskInode * initdiskinodeblock(){
  int i;
  DiskInode *di = (DiskInode*)malloc(sizeof(DiskInode));

  di->inum = 0;
  di->type = 0;
  di->num_of_blocks = 0;
  di->size = 0;

  for(i=0; i<10; i++)
    di->fd.blocks[i] = 0;
  di->ifd = NULL;
  for(i=0; i<FIN; i++)
    di->futureuse[i] = '\0';

  return di;
}

DataBlock * initdatablock(){
  int i;
  DataBlock *db = (DataBlock*)malloc(sizeof(DataBlock));

  for(i=0; i<BUFF; i++)
    db->data[i] = '\0';

  return db;
}
