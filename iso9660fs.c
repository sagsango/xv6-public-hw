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
#include "stat.h"
#include "buf.h"
#include "memlayout.h"
#include "iso9660.h"

  static void 
read_block_range(char* dst, uint start_block, uint count) {
  for (int block=start_block;block<start_block+count;block++) {
      struct buf *bp = bread(2, block);
      memmove(dst,bp->data,BSIZE);
      brelse(bp);
      dst+=BSIZE;
  }
}

// TODO: fill in inode details
  void
iso9660fs_ipopulate(struct inode* ip)
{
    uint addr = ip->inum;
    char buf[2048] = {0};
    begin_op();
    read_block_range(buf,addr/512,4);
    struct iso9660_dir_s *entry = buf + ((int)addr % 512);
    if(entry->length) {
        ip->dev = 2; /* Because inode search for dev = 2 happens */
        //ip->inum = -1;
        //ip->ref = -1;
        //ip->lock = init ?
        //ip->mounted_dev = -1
        ip->type = entry->file_flags == 0 ? T_FILE : T_DIR;
        ip->size = entry->size;
        //ip->mount_parent = 0;
        //ip->mounted_dev = 0;
        //ip->i_func = &iso9660fs_functions;
        //ip->major = 1;
        //ip->minor = 2;
        //ip->addrs[0] = entry->extent*2048;
        ip->flags |= I_VALID;
    }
    end_op();
}

// XXX: write inode details back to disk
  void
iso9660fs_iupdate(struct inode* ip)
{
    cprintf("iso9660fs_iupdate(): %d\n", ip->inum);
}

// XXX: write to file contents
  static int
iso9660fs_writei(struct inode* ip, char* buf, uint offset, uint count)
{
    cprintf("iso9660fs_iwrite(): %d\n", ip->inum);
  return -1;
}


// TODO: read from file contents
  int
iso9660fs_readi(struct inode* ip, char* dst, uint offset, uint size)
{
  if (ip->type == T_DIR) {
    uint addr = ip->addrs[0];
    char buf[2048] = {0};
    begin_op();
    read_block_range(buf,addr/512,4);
    struct iso9660_dir_s *entry = buf + ((int)addr % 512);
    int off;
    for (off = 0; off <= offset; off += sizeof(struct dirent)) {
        if (entry->length) {
            struct dirent *de = dst;
            int trim = 0;
            int i = 0;
            char * c = &entry->filename.str[1];
            for (i=0; i<entry->filename.len; ++i){
                if(c[i] == ';'){
                    trim = entry->filename.len - i;
                    break;
                }
            }
            memset(dst, 0, sizeof(struct dirent));
            memcpy(de->name, &entry->filename.str[1], entry->filename.len - trim);
            de->inum = ip->addrs[0] + ((char*)entry - (char*)buf);
            if(off == offset) {
                end_op();
                return sizeof(struct dirent);
            }
            entry = (struct iso9660_dir_s*) ((char*)entry+entry->length);
        }
        else{
            memset(dst, 0, sizeof(struct dirent));
            end_op();
            return 0;
        }
    }
    end_op();
    return 0;
  }
  else if ( ip->type == T_FILE) {
    uint addr = ip->inum;
    char buf[2048] = {0};
    begin_op();
    read_block_range(buf,addr/512,4);
    struct iso9660_dir_s *entry = buf + ((int)addr % 512);
    if (offset < ip->size) {
      if (offset + size > ip->size) {
          size = ip->size - offset;
      }
      char file_buf[2048] = {0};
      uint data_addr = entry->extent*2048;
      read_block_range(file_buf, data_addr/512, 4);
      memcpy(dst, file_buf + offset, size);
      end_op();
      return size;
    }
    end_op();
    return 0;
  }
  return -1; // return -1 on error
}

struct inode_functions iso9660fs_functions = {
  iso9660fs_ipopulate,
  iso9660fs_iupdate,
  iso9660fs_readi,
  iso9660fs_writei,
};

  void
iso9660fsinit(char * const path, char * device)
{
  /*
    uint dev = ip->dev;
    uint addr = ip->inum;  
    // because we're not sure about the alignment of the record, always read two blocks
    static char buf[1024];
    read_block_range(buf,addr/512,2);
    struct iso9660_dir_s *entry = buf+(addr & 0x1ff); // offset into the block

    ip->type = ((entry->file_flags == 0x2)?T_DIR:T_FILE);
    ip->nlink = 1;
    ip->size = entry->size;
    ip->addrs[0] = entry->extent*2048;

  */
  begin_op();
  struct inode* mount_point = namei(path);
  if (mount_point) {

    static struct iso9660_pvd_s pvd;
    read_block_range(&pvd,64,4);

    if(memcmp(pvd.id,"CD001",5)!=0) {
      cprintf("ID mismatch\n");
      return;
    }
  
    ilock(mount_point);
    mount_point->i_func = &iso9660fs_functions;
    mount_point->mounted_dev = 2;
    mount_point->addrs[0]=pvd.root_directory_record.extent*2048;    
    iunlock(mount_point);
  }
  end_op();
}
