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
#include"datastructs.h"
#include"inits.h"
#include"writes.h"
#include"reads.h"
#include"gets.h"
#include"prints.h"
#include"opens.h"
#include"closes.h"

int main(){
  unsigned int fs_size = 67108864;
  int fd, i, j, k, tok, len, ch;
  char cmd[50], args[10][20], fs_name[] = "sandipfs.bin";
  SuperBlock *sb;
  DiskInode *di;
  DataBlock *db;
  MyDir *dir;

  while(1){
    printf("\nmyshell$: ");
    gets(cmd);
    len = strlen(cmd);
    len++;
    for(i=0, j=0, k=0, tok=0; i<len; i++, j=0, k++, tok++){
      while(cmd[i]!=' ' && i<len)
	i++;
      while(k<i)
	args[tok][j++] = cmd[k++];
      args[tok][j] = '\0';
    }
    printf("%s %s\n",args[0],args[1]);

    if(strcmp(args[0], "mymkfs")==0){
      //      printf("\nEnter the file system size in bytes:- ");
      //      scanf("%u", &fs_size);
      initfilesystem(fs_size,fs_name);
      fd = myopen(fs_name);
      sb = readsuperblock(fd);
      di = iget(sb, fd, 1);
      dir = dirget(sb, fd, di->inum);
    }
    else if(strcmp(args[0], "mymnt")==0){
      fd = myopen(fs_name);
      sb = readsuperblock(fd);
      di = iget(sb, fd, 1);
      dir = dirget(sb, fd, di->inum);
    }
    else if(strcmp(args[0], "mycd")==0){
      namei(sb,di,fd,args[1]);
      di = iget(sb, fd, sb->wd_inode_offset);
      dir = dirget(sb, fd, di->inum);
    }
    else if(strcmp(args[0], "mymkdir")==0){
      mymkdir(sb,di,dir,fd,args[1]);
      di = iget(sb, fd, sb->wd_inode_offset);
      dir = dirget(sb, fd, di->inum);
    }
    else if(strcmp(args[0], "myrmdir")==0){
      myrmdir(sb,di,dir,fd,args[1]);
      di = iget(sb, fd, sb->wd_inode_offset);
      dir = dirget(sb, fd, di->inum);
    }
    else if(strcmp(args[0], "myrmfile")==0){
      myrmfile(sb,di,dir,fd,args[1]);
      di = iget(sb, fd, sb->wd_inode_offset);
      dir = dirget(sb, fd, di->inum);
    }
    else if(strcmp(args[0], "openfile")==0){
      openfile(sb,di,dir,fd,args[1],args[2]);
      di = iget(sb, fd, sb->wd_inode_offset);
      dir = dirget(sb, fd, di->inum);
    }
    else if(strcmp(args[0], "exit")==0){
      writesuperblock(sb,fd);
      exit(0);
    }
    else{
      printf("Error: Invalid command received!!!\n");
      continue;
    }
    printsuperblock(sb);
    printinodeblock(di);
    printdir(di,dir);
    writesuperblock(sb,fd);
    fflush(stdin);
  }

  return 0;
}
