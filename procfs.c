#include "types.h"
#include "defs.h"
#include "file.h"
#include "fs.h"


void procfs_ipopulate(struct inode* ip) {
  ip->size = 0;
  ip->type = 0;
  ip->flags |= I_VALID;
}

void procfs_iupdate(struct inode* ip) {
  cprintf("dummy iupdate\n");
}

int procfs_writei(struct inode* ip, char* buf, uint offset, uint count) {
  cprintf("dummy writei\n");
  return -1;
}

#define PROCFILES 2
struct dirent procfiles[PROCFILES] = {{1,"meminfo"}, {2,"diskinfo"}};

int procfs_readi(struct inode* ip, char* buf, uint offset, uint size) {    
  if(ip->mounted_dev) {
    int end = size+offset;
    if(end > sizeof(procfiles))
      end = sizeof(procfiles);
    memmove(buf,((char*)procfiles)+offset,end-offset);
    return end-offset;
  }
  else {
    char *hw = "Hello World!\n";
    memmove(buf,hw,strlen(hw));
    if(offset==0)
      return strlen(hw);
    else
      return 0;
  }
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

