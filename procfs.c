#include "types.h"
#include "param.h"
#include "defs.h"
#include "file.h"
#include "fs.h"
#include "spinlock.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

void procfs_ipopulate(struct inode* ip) {
  ip->size = 0;
  if(ip->inum < 10000)
    ip->type = 1;
  else
    ip->type = 0;

  ip->flags |= I_VALID;
}

void procfs_iupdate(struct inode* ip) {
//  cprintf("dummy iupdate\n");
}

int procfs_writei(struct inode* ip, char* buf, uint offset, uint count) {
  cprintf("dummy writei\n");
  return -1;
}

#define PROCFILES 2
struct dirent procfiles[NPROC+PROCFILES] = {{10001,"meminfo"}, {10002,"diskinfo"}};


static void
sprintint(char* buf, uint x) 
{
  static char digits[] = "0123456789";
  int index=0;
  if(x>10) buf[index++]=digits[(x/10)%10];
  buf[index++]=digits[x%10];
  buf[index++]=0;
}

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

// returns the number of active processes, and updates the procfiles table
static int updateprocfiles() {
  int num = 0, index = 0;
  acquire(&ptable.lock);
  while(index < NPROC) {
    if(ptable.proc[index].state != UNUSED && ptable.proc[index].state != ZOMBIE) { 
      procfiles[num+PROCFILES].inum = index+1;
      sprintint(procfiles[num+PROCFILES].name,ptable.proc[index].pid);
      num++;
    }
    index++;
  }
  release(&ptable.lock);
  return num+PROCFILES;
}

int procfs_readi(struct inode* ip, char* buf, uint offset, uint size) 
{    
  int procsize = sizeof(struct dirent)*updateprocfiles();
  if(ip->mounted_dev) {
    int end = size+offset;
    if(end > procsize)
      end = procsize;
    memmove(buf,((char*)procfiles)+offset,end-offset);
    return end-offset;
  }
  // directory - can only be one of the process directories
  else if(ip->type == 1) {
    struct dirent procdir[3] = {{20000+ip->inum,"name"},{30000+ip->inum,"parent"}};
    int end = size+offset;
    if(end > sizeof(procdir))
      end = sizeof(procdir);
    memmove(buf,((char*)procdir)+offset,end-offset);
    return end-offset;
  }
  // file
  else {
    char *hw = "Hello World!\n";

    switch(((int)ip->inum)) {
    case 10001: case 10002:
      memmove(buf,hw,strlen(hw));
      if(offset==0)
        return strlen(hw);
      else
        return 0;
    }
    
    switch(((int)ip->inum/10000)) {
    case 2:
      memmove(buf,ptable.proc[ip->inum-20001].name,16);      
      if(offset>=strlen(buf)) return 0;
      return strlen(buf);
    case 3:
      sprintint(buf,ptable.proc[ip->inum-30001].parent->pid);
      if(offset>=strlen(buf)) return 0;
      return strlen(buf);    
    }
  }
  return 0;
}

struct inode_functions procfs_functions = { 
  procfs_ipopulate, 
  procfs_iupdate,
  procfs_readi,
  procfs_writei 
};

void procfsinit() {
  begin_op();
  struct inode* mount_point = namei("/proc");
  if(mount_point) {
    ilock(mount_point); 
    mount_point->i_func = &procfs_functions;
    mount_point->mounted_dev = 2;
    iunlock(mount_point);
  }
  end_op();
}

