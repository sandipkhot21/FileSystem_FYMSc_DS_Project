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

int myopen(char *filename){
  return(open(filename, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO));
}

DiskInode *ialloc(SuperBlock *sb, int fd){
  unsigned int inum, offset;
  DiskInode *di = NULL;

  if(sb->no_of_free_inodes == 0){
    printf("Error: No more free inodes available!!!\n");
    return di;
  }
  if(sb->no_of_free_datablocks == 0){
    printf("Error: No more free datablocks available!!!\n");
    return di;
  }

  inum = sb->next_free_inode_offset;

  sb->i_bmap[sb->next_free_inode_offset] = '1';
  while(sb->i_bmap[sb->next_free_inode_offset]!='\0'){
    if(sb->next_free_inode_offset == NIN)
      sb->next_free_inode_offset = 1;
    sb->next_free_inode_offset++;
  }

  sb->no_of_free_inodes--;
  sb->next_free_inode_index = sb->inodeblock_index+sizeof(DiskInode)*sb->next_free_inode_offset;

  di = initdiskinodeblock();
  di->inum = inum;
  di->fd.blocks[0] = sb->next_free_datablock_index;
  di->num_of_blocks = 1;
  di->size = 0;

  sb->db_bmap[sb->next_free_datablock_offset] = '1';
  while(sb->db_bmap[sb->next_free_datablock_offset] != '\0'){
    if(sb->next_free_datablock_offset == NDB)
      sb->next_free_datablock_offset = 1;
    sb->next_free_datablock_offset++;
  }
  sb->no_of_free_datablocks--;
  sb->next_free_datablock_index = sb->datablock_index+sizeof(DataBlock)*sb->next_free_datablock_offset;

  sb->flag = 1;
  
  return di;
}

void mymkdir(SuperBlock *sb, DiskInode *di, MyDir *dir, int fd, char *fname){
  int i, j, len, offset;
  DiskInode *ndi = NULL;
  MyDir *ndir = NULL, *tdir;

  ndi = ialloc(sb, fd);
  ndi->type = DIR;
  ndi->size = sizeof(MyDir)*2;
  ndir = (MyDir*)malloc(sizeof(MyDir)*2);
  ndir->inum = ndi->inum;
  strcpy(ndir->fname,".");
  (ndir+sizeof(MyDir))->inum = di->inum;
  strcpy((ndir+sizeof(MyDir))->fname,"..");

  di->size += sizeof(MyDir);
  tdir = (MyDir*)malloc(di->size);
  for(i=0; i<(di->size-sizeof(MyDir)); i+=sizeof(MyDir)){
    (tdir+i)->inum = (dir+i)->inum;
    strcpy((tdir+i)->fname,(dir+i)->fname);
  }
  free(dir);
  dir = tdir;

  (dir+di->size-sizeof(MyDir))->inum = ndi->inum;
  strcpy((dir+di->size-sizeof(MyDir))->fname,fname);
  writedir(sb,di,fd,dir);
  iput(sb,di,fd);

  writedir(sb,ndi,fd,ndir);
  iput(sb,ndi,fd);
}

DiskInode *namei(SuperBlock *sb, DiskInode *di, int fd, char*path){
  int i, j, k, len, no_of_entries, offset;
  char arr[16];
  MyDir *dir;
  len = strlen(path);

  if(path[0] == '/'){
    di = iget(sb, fd, 1);
		i = 1;
  }
  else
    i = 0;
  while(len>0){
    for(i, j=0; path[i]!='/' && path[i]!='\0'; i++, j++)
      arr[j] = path[i];
    arr[j++]='\0';
    i++;
    len -= j;

    if(di->type != DIR)
      printf("Error: Invalid path specified!!!\n");
    if(di->inum==1 && strcmp(arr,"..")==0)
      continue;
    if(strcmp(arr,".")==0)
      continue;
    
    dir = dirget(sb,fd,di->inum);
    k = di->size;
    for(j=0; j<k; j+=sizeof(MyDir)){
      if(strcmp((dir+j)->fname,arr)==0){
	di = iget(sb,fd,(dir+j)->inum);
	sb->wd_inode_offset = di->inum;
	sb->wd_inode_index = sb->inodeblock_index+di->inum*sizeof(DiskInode);
	sb->wd_db_index = di->fd.blocks[0];
	break;
      }
    }
    if(j==k)
      printf("Error: incorrect path name!!!\n", j, arr);
  }
  return di;
}

int openfile(SuperBlock *sb, DiskInode *di, MyDir *dir, int fd, char *fname, char *mode){
  int i, j, finum, no_of_entries, num_of_blocks, size=0, offset, ch=0;
  DiskInode *fi;
  char *buf = (char*)malloc(BUFF);
  char **abuf = (char**)malloc(sizeof(char*)*10);
  
  no_of_entries = di->size/sizeof(MyDir);
  for(i=0; i<no_of_entries; i++)
    if(strcmp((dir+i*sizeof(MyDir))->fname,fname) == 0){
      finum = (dir+i*sizeof(MyDir))->inum;
      fi = iget(sb,fd,finum);
      fi->type = REGF;
      break;
    }
  if(i==no_of_entries){
    //    printf("\nFile does not exist. Create new file?y/n: - ");
    //    scanf("%c", &ch);
    fi = ialloc(sb,fd);
    fi->type = REGF;
    iput(sb,fi,fd);
  }
  if(*mode == 'r'){
    printf("\nFile Data is:- \n");
    for(i=0; i<fi->num_of_blocks; i++){
      offset = lseek(fd,fi->fd.blocks[i],SEEK_SET);
      if(offset == -1)
	printf("Error: Unable to seek file pointer!!!\n");
      myread(fd,buf,BUFF);
      for(j=0; j<BUFF && buf[j]!='#'; j++)
	printf("%c",buf[j]);
    }
    return 0;
  }
  if(*mode == 'w'){
    printf("\nEnter the file data below :-\n");
    for(i=0; i<10 && ch!='#'; i++){
      for(j=0; j<4096 && ch!='#'; j++){
	ch = getc(stdin);
	*(buf+j) = (char)ch;
      }
      *(abuf+i) = buf;
      size += j;
    }
    fi->size = size;
    fi->num_of_blocks = i;
    iput(sb,fi,fd);
    myfwrite(sb,fi,fd,abuf);
    mymkfile(sb,di,fi,dir,fd,fname);
    if(i>=10 && j>=4096)
      printf("\nError: Unable to accept more data, Max file size reached!!!");
    return 0;
  }
}

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
