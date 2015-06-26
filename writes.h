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


void mywrite(int fd, void *buf, int n){
  write(fd, buf, n);
}

void writesuperblock(SuperBlock *sb, int fd){
  int i, offset;

  offset = lseek(fd, 0, SEEK_SET);
  if(offset == -1)
    printf("Error: Unable to seek file pointer!!!\n");

  mywrite(fd,&(sb->fs_size),sizeof(sb->fs_size));
  mywrite(fd,&(sb->total_num_of_inodes),sizeof(sb->total_num_of_inodes));
  mywrite(fd,&(sb->no_of_free_inodes),sizeof(sb->no_of_free_inodes));
  mywrite(fd,&(sb->next_free_inode_index),sizeof(sb->next_free_inode_index));
  mywrite(fd,&(sb->next_free_inode_offset),sizeof(sb->next_free_inode_offset));
  mywrite(fd,&(sb->inodeblock_index),sizeof(sb->inodeblock_index));
  mywrite(fd,&(sb->total_num_of_datablocks),sizeof(sb->total_num_of_datablocks));
  mywrite(fd,&(sb->no_of_free_datablocks),sizeof(sb->no_of_free_datablocks));
  mywrite(fd,&(sb->next_free_datablock_index),sizeof(sb->next_free_datablock_index));
  mywrite(fd,&(sb->next_free_datablock_offset),sizeof(sb->next_free_datablock_offset));
  mywrite(fd,&(sb->datablock_index),sizeof(sb->datablock_index));
  mywrite(fd,&(sb->root_inode_index),sizeof(sb->root_inode_index));
  mywrite(fd,&(sb->root_db_index),sizeof(sb->root_db_index));
  mywrite(fd,&(sb->wd_inode_offset),sizeof(sb->wd_inode_offset));
  mywrite(fd,&(sb->wd_inode_index),sizeof(sb->wd_inode_index));
  mywrite(fd,&(sb->wd_db_index),sizeof(sb->wd_db_index));
  mywrite(fd,&(sb->flag),sizeof(sb->flag));
  for(i=0; i<FSB; i++)
    mywrite(fd,&(sb->futureuse[i]),sizeof(sb->futureuse[i]));
  for(i=0; i<NIN; i++)
    mywrite(fd,&(sb->i_bmap[i]),sizeof(sb->i_bmap[i]));
  for(i=0; i<NDB; i++)
    mywrite(fd,&(sb->db_bmap[i]),sizeof(sb->db_bmap[i]));
}

void iput(SuperBlock *sb, DiskInode *di, int fd){
  int i=0, offset;

  offset = lseek(fd, (sb->inodeblock_index+di->inum*sizeof(DiskInode)), SEEK_SET);
  if(offset == -1)
    printf("Error: Unable to seek file pointer!!!\n");
  
  mywrite(fd,&(di->inum),sizeof(di->inum));
  mywrite(fd,&(di->type),sizeof(di->type));
  mywrite(fd,&(di->num_of_blocks),sizeof(di->num_of_blocks));
  mywrite(fd,&(di->size),sizeof(di->size));
  for(i=0; i<10; i++)
    mywrite(fd,&(di->fd.blocks[i]),sizeof(di->fd.blocks[i]));
  mywrite(fd,&(di->ifd),sizeof(di->ifd));
  for(i=0; i<FIN; i++)
    mywrite(fd,&(di->futureuse[i]),sizeof(di->futureuse[i]));
}

void writedatablock(SuperBlock *sb, DataBlock *db, int fd){
  int i;

  for(i=0; i<BUFF; i++)
    mywrite(fd, &(db->data[i]), sizeof(db->data[i]));
}

void writedir(SuperBlock *sb, DiskInode *di, int fd, MyDir *dir){
  int i, j, k, no_of_blocks_required, offset;
  int no_of_entries = di->size/sizeof(MyDir);

  no_of_blocks_required = (di->size/BUFF);

  for(i=0, k=0; i<=no_of_blocks_required; i++){

    if(di->fd.blocks[i] == 0){
      di->fd.blocks[i] = dbget(sb, fd);
    }

    offset = lseek(fd, di->fd.blocks[i], SEEK_SET);
    if(offset == -1)
      printf("Error: Unable to seek file pointer!!!\n");

    for(j=0; j<BUFF && k<di->size; j+=sizeof(MyDir), k+=sizeof(MyDir)){
      mywrite(fd, &((dir+j)->inum), sizeof((dir+j)->inum));
      mywrite(fd, &((dir+j)->fname), sizeof((dir+j)->fname));
    }
  }
}
void mymkfile(SuperBlock *sb, DiskInode *di, DiskInode *fi, MyDir *dir, int fd, char *fname){
  int i, j, len, no_of_entries, offset, osize, nsize;
  MyDir *tdir;

  no_of_entries = di->size/sizeof(MyDir);
  osize = di->size;
  nsize = osize + sizeof(MyDir);
  di->size = nsize;

  tdir = (MyDir*)malloc(nsize);
  for(i=0; i<osize; i+=sizeof(MyDir)){
    (tdir+i)->inum = (dir+i)->inum;
    strcpy((tdir+i)->fname,(dir+i)->fname);
  }
  free(dir);
  dir = tdir;

  (dir+osize)->inum = fi->inum;
  strcpy((dir+osize)->fname,fname);

  writedir(sb,di,fd,dir);
  iput(sb,di,fd);
}

void myfwrite(SuperBlock *sb, DiskInode *fi, int fd, char **buf){
  int i, j, offset;

  for(i=0; i<fi->num_of_blocks; i++){
    if(fi->fd.blocks[i] == 0)
      fi->fd.blocks[i] = dbget(sb,fd);
    offset = lseek(fd, fi->fd.blocks[i], SEEK_SET);
    j = (fi->size - i*BUFF);
    mywrite(fd,buf[i],j);
  }
}

int myrmdir(SuperBlock *sb, DiskInode *di, MyDir *dir, int fd, char *fname){
  int i, j, temp, no_of_entries, offset;
  MyDir *tdir;
  DiskInode *tdi, *tdi1;
  DataBlock *db;

  no_of_entries = di->size/sizeof(MyDir);
  for(i=0; i<no_of_entries; i++){
    if(strcmp((dir+i*sizeof(MyDir))->fname,fname) == 0){
      tdi = iget(sb,fd,(dir+i*sizeof(MyDir))->inum);
      for( ; i<no_of_entries-1; i++){
	(dir+i*sizeof(MyDir))->inum = (dir+(i+1)*sizeof(MyDir))->inum;
	strcpy((dir+i*sizeof(MyDir))->fname,(dir+(i+1)*sizeof(MyDir))->fname);
	j = strlen((dir+i*sizeof(MyDir))->fname);
	for( ; j<16; j++)
	  (dir+i*sizeof(MyDir))->fname[j] = '\0';
      }
      (dir+i*sizeof(MyDir))->inum = 0;
      for(j=0; j<16; j++)
	(dir+i*sizeof(MyDir))->fname[j] = '\0';
      di->size-=sizeof(MyDir);

      writedir(sb,di,fd,dir);
      iput(sb,di,fd);

      sb->i_bmap[tdi->inum] = '\0';
      sb->no_of_free_inodes++;
      sb->next_free_inode_offset = tdi->inum;

      db = initdatablock();
      sb->next_free_datablock_index = tdi->fd.blocks[0];
      for(j=0; j<tdi->num_of_blocks; j++){
	sb->db_bmap[(tdi->fd.blocks[j]-sb->datablock_index)/BUFF] = '\0';

	offset = lseek(fd, tdi->fd.blocks[j], SEEK_SET);
	if(offset == -1)
	  printf("Error: Unable to seek file pointer!!!\n");
	writedatablock(sb, db, fd);
      }
      sb->no_of_free_datablocks += tdi->num_of_blocks;
      tdi1 = initdiskinodeblock();
      tdi1->inum = tdi->inum;
      free(tdi);
      iput(sb, tdi1, fd);
      return 1;
    }
  }
  return 0;
}

int myrmfile(SuperBlock *sb, DiskInode *di, MyDir *dir, int fd, char *fname){
  int i, j, temp, no_of_entries, offset;
  MyDir *tdir;
  DiskInode *fi, *fi1;
  DataBlock *db;

  no_of_entries = di->size/sizeof(MyDir);
  for(i=0; i<no_of_entries; i++){
    if(strcmp((dir+i*sizeof(MyDir))->fname,fname) == 0){
      fi = iget(sb,fd,(dir+i*sizeof(MyDir))->inum);
      for( ; i<no_of_entries-1; i++){
	(dir+i*sizeof(MyDir))->inum = (dir+(i+1)*sizeof(MyDir))->inum;
	strcpy((dir+i*sizeof(MyDir))->fname,(dir+(i+1)*sizeof(MyDir))->fname);
	j = strlen((dir+i*sizeof(MyDir))->fname);
	for( ; j<16; j++)
	  (dir+i*sizeof(MyDir))->fname[j] = '\0';
      }
      (dir+i*sizeof(MyDir))->inum = 0;
      for(j=0; j<16; j++)
	(dir+i*sizeof(MyDir))->fname[j] = '\0';
      di->size-=sizeof(MyDir);

      writedir(sb,di,fd,dir);
      iput(sb,di,fd);

      sb->i_bmap[fi->inum] = '\0';
      sb->no_of_free_inodes++;
      sb->next_free_inode_offset = fi->inum;

      db = initdatablock();
      sb->next_free_datablock_index = fi->fd.blocks[0];
      for(j=0; j<fi->num_of_blocks; j++){
	sb->db_bmap[(fi->fd.blocks[j]-sb->datablock_index)/BUFF] = '\0';

	offset = lseek(fd, fi->fd.blocks[j], SEEK_SET);
	if(offset == -1)
	  printf("Error: Unable to seek file pointer!!!\n");
	writedatablock(sb, db, fd);
      }
      sb->no_of_free_datablocks += fi->num_of_blocks;
      fi1 = initdiskinodeblock();
      fi1->inum = fi->inum;
      free(fi);
      iput(sb, fi1, fd);
      return 1;
    }
  }
  return 0;
}
