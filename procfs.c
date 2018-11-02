#include "types.h"
#include "param.h"
#include "defs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fs.h"
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

  void
procfs_iupdate(struct inode* ip)
{
}

  static int
procfs_writei(struct inode* ip, char* buf, uint offset, uint count)
{
  return -1;
}

#define PROCFILES ((2))
struct dirent procfiles[PROCFILES+1+NPROC] = {{10001,"meminfo"}, {10002,"cpuinfo"}};

static void
sprintuint(char* buf, uint x)
{
  uint stack[10];
  uint stack_size = 0;
  if (x == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  while (x) {
    stack[stack_size++] = x % 10u;
    x /= 10u;
  }
  uint buf_size = 0;
  while (stack_size) {
    buf[buf_size++] = '0' + stack[stack_size - 1];
    stack_size--;
  }
  buf[buf_size] = 0;
}

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

// returns the number of active processes, and updates the procfiles table
static uint updateprocfiles() {
  int num = 0, index = 0;
  
  acquire(&ptable.lock);
  while(index < NPROC) {
    if(ptable.proc[index].state != UNUSED && ptable.proc[index].state != ZOMBIE) {
      procfiles[PROCFILES+1+num].inum = index+1;
      sprintuint(procfiles[PROCFILES+1+num].name,ptable.proc[index].pid);
      num++;
      if (ptable.proc[index].pid == proc->pid) {
        procfiles[PROCFILES].inum = index+1;
        memmove(procfiles[PROCFILES].name,"self",5);
      }
    }
    index++;
  }
  release(&ptable.lock);
  return PROCFILES + 1 + num;
}

  static int
readi_helper(char * buf, uint offset, uint maxsize, char * src, uint srcsize)
{
  if (offset > srcsize)
    return -1;
  uint end = offset + maxsize;
  if (end > srcsize)
    end = srcsize;
  memmove(buf, src+offset, end-offset);
  return end-offset;
}


  int
procfs_readi(struct inode* ip, char* buf, uint offset, uint size)
{
  const uint procsize = sizeof(struct dirent)*updateprocfiles();
  char buf1[20];
  if(ip->mounted_dev) { // the mount point
    return readi_helper(buf, offset, size, (char *)procfiles, procsize);
  } else if (ip->type == 1) {  // directory - can only be one of the process directories
    struct dirent procdir[3] = {{20000+ip->inum, "name"}, {30000+ip->inum, "parent"}, {40000+ip->inum, "pid"}};
    return readi_helper(buf, offset, size, (char *)procdir, sizeof(procdir));
  } else {  // files
    switch(((int)ip->inum)) {
    case 10001: // meminfo
      sprintuint(buf1, kmemfreecount());
      return readi_helper(buf, offset, size, buf1, strlen(buf1));
    case 10002:
      sprintuint(buf1, ncpu);
      return readi_helper(buf, offset, size, buf1, strlen(buf1));
    default: break;
    }

    switch(((int)ip->inum/10000)) {
    case 2:
      return readi_helper(buf, offset, size, ptable.proc[ip->inum-20001].name, 16);
    case 3:
      sprintuint(buf1,ptable.proc[ip->inum-30001].parent->pid); // see updateprocfiles()
      return readi_helper(buf, offset, size, buf1, strlen(buf1));
    case 4:
      sprintuint(buf1,ptable.proc[ip->inum-40001].pid); // see updateprocfiles()
      return readi_helper(buf, offset, size, buf1, strlen(buf1));
    default:
      break;
    }
  }
  return -1;
}

struct inode_functions procfs_functions = {
  procfs_ipopulate,
  procfs_iupdate,
  procfs_readi,
  procfs_writei
};

  void
procfsinit(char * const path) {
  begin_op();
  struct inode* mount_point = namei(path);
  if(mount_point) {
    ilock(mount_point);
    mount_point->i_func = &procfs_functions;
    mount_point->mounted_dev = 2;
    iunlock(mount_point);
  }
  end_op();
}
