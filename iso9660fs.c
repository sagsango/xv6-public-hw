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

    ip->type = T_FILE;
    ip->size = 100;
    ip->flags |= I_VALID;

    cprintf("iso9660fs_ipopulate(): %d\n", ip->inum);
    return;
    /*

        you know the offset to read from
        get details from there.
    */
    uint addr = ip->inum;
    char buf[2048] = {0};
    begin_op();
    read_block_range(buf,addr/512,4);
    struct iso9660_dir_s *entry = buf;
    if(entry->length) {
        ip->type = entry->file_flags == 0 ? T_FILE : T_DIR;
        ip->major = 2;
        ip->minor = 0; // I dont know
        ip->nlink = 1;
        if(ip->type == T_FILE) {
            ip->size = entry->size;
        }else{
            char file_buf[2048] = {0};
            uint data_addr = entry->extent*2048;
            read_block_range(file_buf, data_addr/512, 4);
            struct iso9660_dir_s *my_entry = file_buf;
            int cnt = 0;
            while (my_entry->length) {
                cprintf("llop\n");
                cnt += 1;
                my_entry = (struct iso9660_dir_s*) ((char*)my_entry+my_entry->length);
            }
            ip->size = sizeof(struct dirent) * cnt;
        }
        // ip->mount_parent = 0;
        // ip->mounted_dev = 0;
        ip->addrs[0] = entry->extent*2048;
        ip->flags |= I_VALID;
    }
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
  if ( ip->type == T_DIR ) {
    uint addr = ip->addrs[0];
    char buf[2048] = {0};
    begin_op();
    read_block_range(buf,addr/512,4);
    struct iso9660_dir_s *entry = buf;
    //cprintf("iso9660fs_readi(1): inode_num:%d addr:%d, offset:%d, size:%d\n", ip->inum, ip->addrs[0], offset, size);
    int off;
    for (off = 0; off <= offset; off += sizeof(struct dirent)) {
        //cprintf("Here(1)\n");
        if (entry->length) {
            struct dirent *de = dst;
            char namebuf[100]={0};
            memset(dst, 0, sizeof(struct dirent));
            memcpy(de->name, &entry->filename.str[1], entry->filename.len);
            memcpy(namebuf,&entry->filename.str[1],entry->filename.len);
            de->inum = ip->addrs[0] + ((char*)entry - (char*)buf);
            /*cprintf("iso9660fs_readi(): name:%s\n", namebuf);
            cprintf("Entry: %s\n",namebuf);
            cprintf("length:%d\n"
                    "xa_length:%d\n"
                    "extent:%p\n"
                    "size:%d\n"
                    "file_flags:%d\n"
                    "file_unit_size:%d\n"
                    "interleave_gap:%d\n"
                    "volume_sequence_number:%d\n\n\n",
                    entry->length,
                    entry->xa_length,
                    entry->extent,
                    entry->size,
                    entry->file_flags,
                    entry->file_unit_size,
                    entry->interleave_gap,
                    entry->volume_sequence_number); */
            //return sizeof(struct dirent);
            //return entry->length;
            if(off == offset) {
                //cprintf("Here(2)\n");
                end_op();
                return sizeof(struct dirent);
            }
            entry = (struct iso9660_dir_s*) ((char*)entry+entry->length);
        }
        else{
            //cprintf("Here(3)\n");
            memset(dst, 0, sizeof(struct dirent));
            end_op();
            return 0;
        }
        //cprintf("loop(2)\n");
    }
    //cprintf("Here(4)\n");
    end_op();
    return 0;
    if(offset==0) {
      struct dirent *de = dst;
      memmove(de->name,"FAKE.txt",9);
      de->inum = 1;
      return sizeof(struct dirent);
    }
    else {
      return 0;
    }
  }
  else if ( ip->type == T_FILE) {
    int len = size<16?size:16;
    cprintf("iso9660fs_readi(2): inode_num:%d addr:%d, offset:%d, size:%d\n", ip->inum, ip->addrs[0], offset, size); 
    if (offset < ip->size) {
      memmove(dst,"Very Fake Text.",len);
      return len;
    }
    else {
      return 0;
    }
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
  cprintf("Mounting cdrom\n");
  begin_op();
  struct inode* mount_point = namei(path);
  if (mount_point) {

    static struct iso9660_pvd_s pvd;
    read_block_range(&pvd,64,4);
    if(memcmp(pvd.id,"CD001",5)==0) {
      cprintf("ID matched ISO9660 file system\n");
    }
    else {
      cprintf("ID mismatch\n");
      return;
    }
  
  
    cprintf("Root directory record version %d len %d size %d at %d, volume size %d block size %d set size %d.\n",pvd.version,
    pvd.root_directory_record.length,
    pvd.root_directory_record.size,
    pvd.root_directory_record.extent,pvd.volume_space_size*2048,pvd.logical_block_size,pvd.volume_set_size);
    
    uint addr = pvd.root_directory_record.extent*2048; // location of directory file in bytes      
    static char buf[2048];
    read_block_range(buf,addr/512,4);
    struct iso9660_dir_s *entry = buf;

    /* XXX: cmd = mount iso9660 cdrom mnt */
    while (entry->length) {
      char namebuf[100]={0};
      memcpy(namebuf,&entry->filename.str[1],entry->filename.len);
      cprintf("Entry: %s\n",namebuf);
      cprintf("length:%d\n"
              "xa_length:%d\n"
              "extent:%p\n"
              "size:%d\n"
              "file_flags:%d\n"
              "file_unit_size:%d\n"
              "interleave_gap:%d\n"
              "volume_sequence_number:%d\n\n\n",
              entry->length,
              entry->xa_length,
              entry->extent,
              entry->size,
              entry->file_flags,
              entry->file_unit_size,
              entry->interleave_gap,
              entry->volume_sequence_number);
      if(entry->file_flags == 0){
          cprintf("File_content:");
          char file_buf[2048] = {0};
          int file_len = entry->size;
          uint data_addr = entry->extent*2048;
          read_block_range(file_buf, data_addr/512, 4);
          file_buf[file_len] = 0;
          cprintf("%s\n", file_buf);
          cprintf("Len_was:%d\n", entry->size);
      }
      else if(entry->file_flags == 2) {
          char file_buf[2048] = {0};
          uint data_addr = entry->extent*2048;
          read_block_range(file_buf, data_addr/512, 4);
          struct iso9660_dir_s *my_entry = file_buf;
          uint index = entry->extent*2048;
          while (my_entry->length) {
              char my_namebuf[100]={0};
              memcpy(my_namebuf,&my_entry->filename.str[1],my_entry->filename.len);
              cprintf("             Inside_Entry[at_idx:%d]: %s\n",index, my_namebuf);
              index += my_entry->length;
              my_entry = (struct iso9660_dir_s*) ((char*)my_entry+my_entry->length);
          }

      }
      entry = (struct iso9660_dir_s*) ((char*)entry+entry->length);
    }


    ilock(mount_point);
    mount_point->i_func = &iso9660fs_functions;
    mount_point->mounted_dev = 2;
    mount_point->addrs[0]=pvd.root_directory_record.extent*2048;    
    cprintf("iso9660fs_init(): %d, addr:%d\n", mount_point->inum, mount_point->addrs[0]);
    iunlock(mount_point);
  }
  end_op();
}
