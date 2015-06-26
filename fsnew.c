#define BUFF 4096
#define NIN 12288
#define NDB 12288
#define FIN 66
#define FSB 4034
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
  int fd;
  char fs_name[] = "sandipfs.bin";
  char path[] = ".././.././..";
  SuperBlock *sb;
  DiskInode *di;
  DataBlock *db;
  MyDir *dir;

  //  initfilesystem(fs_size,fs_name);
  fd = myopen(fs_name);
  sb = readsuperblock(fd);
  di = iget(sb, fd, 1);
  dir = dirget(sb, fd, di->inum);

  openfile(sb,di,dir,fd,"sandip.txt",'r');

  di = iget(sb, fd, 1);
  dir = dirget(sb, fd, di->inum);

  printsuperblock(sb);
  printinodeblock(di);
  printdir(di,dir);


  writesuperblock(sb,fd);
  return 0;
}
