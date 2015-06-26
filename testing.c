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

  fd = myopen(fs_name);
  sb = readsuperblock(fd);
  di = iget(sb, fd, 1);
  dir = dirget(sb, fd, di->inum);

  myrmdir(sb,di,dir,fd,"sandip2");
  di = iget(sb, fd, 1);
  dir = dirget(sb, fd, di->inum);

  printsuperblock(sb);
  printinodeblock(di);
  printdir(di,dir);

  return 0;
}
