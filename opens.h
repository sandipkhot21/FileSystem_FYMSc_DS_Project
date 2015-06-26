#define BUFF 4096
#define NIN 12288
#define NDB 12288
#define FIN 66
#define FSB 4030
#define MFS 40960
#define DIR 1
#define REGF 2

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<malloc.h>
#include<stdlib.h>
#include<string.h>


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
