#define BUFF 4096
#define NIN 12288
#define NDB 12288
#define FIN 66
#define FSB 4034
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

int main(){
  int i;
  SuperBlock *sb;
  DiskInode *di;
  MyDir *dir;
  
  
  return 0;
}
