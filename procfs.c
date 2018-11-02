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
#include "memlayout.h"

#define T_DIR ((1))

  void
procfs_ipopulate(struct inode* ip)
{
  ip->size = 0;
  ip->flags |= I_VALID;

  // inum < 10000 are reserved for directories
  // use inum > 10000 for files in procfs
  ip->type = ip->inum < 10000 ? T_DIR : 100;
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

  static void
sprintx32(char * buf, uint x)
{
  buf[0] = x >> 28;
  for (int i = 0; i < 8; i++) {
    uint y = 0xf & (x >> (28 - (i * 4)));
    buf[i] = (y < 10) ? (y + '0') : (y + 'a' - 10);
  }
}

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

#define PROCFILES ((2))
struct dirent procfiles[PROCFILES+NPROC+1] = {{10001,"meminfo"}, {10002,"cpuinfo"}};

// returns the number of active processes, and updates the procfiles table
  static uint
updateprocfiles()
{
  int num = 0, index = 0;
  acquire(&ptable.lock);
  while (index < NPROC) {
    if (ptable.proc[index].state != UNUSED && ptable.proc[index].state != ZOMBIE) {
      procfiles[PROCFILES+num].inum = index+1;
      sprintuint(procfiles[PROCFILES+num].name,ptable.proc[index].pid);
      num++;
      // also checks if the process at [index] is the current process
      // if yes, create a "self" directory
      // TODO: your code here


    }
    index++;
  }
  release(&ptable.lock);
  return PROCFILES + num;
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
  // the mount point
  if (ip->mounted_dev == 2) {
    return readi_helper(buf, offset, size, (char *)procfiles, procsize);
  }

  // directory - can only be one of the process directories
  if (ip->type == T_DIR) {
    // List the files in a process directory:
    // It contains "name", "pid", "ppid", and "mappings".
    // Choose a good pattern for inum.
    // You will need to check inum to see what should be the file content (see below)
    // TODO: Your code here


    return -1; // remove this after implementation
  }

  // files
  char buf1[32];
  switch (((int)ip->inum)) {
    case 10001: // meminfo: print the number of free pages
      sprintuint(buf1, kmemfreecount());
      return readi_helper(buf, offset, size, buf1, strlen(buf1));
    case 10002: // cpuinfo: print the total number of cpus. See the 'ncpu' global variable
      // TODO: Your code here


      return -1; // remove this after implementation
    default: break;
  }

  // filling the content for all other files
  // TODO: Your code here



  return -1; // return -1 on error
}

struct inode_functions procfs_functions = {
  procfs_ipopulate,
  procfs_iupdate,
  procfs_readi,
  procfs_writei,
};

  void
procfsinit(char * const path)
{
  begin_op();
  struct inode* mount_point = namei(path);
  if (mount_point) {
    ilock(mount_point);
    mount_point->i_func = &procfs_functions;
    mount_point->mounted_dev = 2;
    iunlock(mount_point);
  }
  end_op();
}
