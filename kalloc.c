// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file

struct frameinfo {
  int refs;
  addr_t checksum; // this is not guaranteed to be correct, should be refreshed before use
};
struct frameinfo frameinfo [PHYSTOP/PGSIZE];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  memset(frameinfo, 0, sizeof(frameinfo));
  kmem.use_lock = 0;
  kmem.freelist = 0;
  freerange(vstart, vend);
}

void
kinit2()
{
  kmem.use_lock = 1;
}

  void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((addr_t)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
  void
kfree(char *v)
{
  struct run *r;

  if((addr_t)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
  char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  if(kmem.use_lock)
    release(&kmem.lock);

  frameinfo[PGINDEX(V2P(r))].refs = 1;
  return (char*)r;
}

// release frame with kernel virtual address v
  void
krelease(char *v)
{
  addr_t frame = V2P(v);
  frameinfo[PGINDEX(frame)].refs--;
  if (frameinfo[PGINDEX(frame)].refs == 0)
    kfree(P2V(frame));
}

void
kreleasen(char *v, int n)
{
  addr_t frame = V2P(v);
  frameinfo[PGINDEX(frame)].refs -= n;
  if (frameinfo[PGINDEX(frame)].refs == 0)
    kfree(P2V(frame));
}

  void
kretain(char *v)
{
  addr_t frame = V2P(v);
  frameinfo[PGINDEX(frame)].refs++;
}

  void
kretainn(char *v, int n)
{
  addr_t frame = V2P(v);
  frameinfo[PGINDEX(frame)].refs += n;
}

  int
krefcount(char *v)
{
  return frameinfo[PGINDEX(V2P(v))].refs;
}

  void
update_checksum(addr_t frame)
{
  struct frameinfo *f = &frameinfo[PGINDEX(frame)];
  f->checksum = 0;

  addr_t *v = P2V(frame);
  for (addr_t *i=v; i<v+PGSIZE/8; i++)
    f->checksum+=*i;
}

// this should only be called after all checksums have been updated
  int
frames_are_identical(addr_t frame1, addr_t frame2)
{
  return frameinfo[PGINDEX(frame1)].checksum == frameinfo[PGINDEX(frame2)].checksum &&
    memcmp(P2V(frame1),P2V(frame2),PGSIZE)==0;
}

  int
kfreepagecount()
{
  int i=0;

  acquire(&kmem.lock);
  struct run *list = kmem.freelist;
  while(list) {
    i++;
    list=list->next;
  }
  release(&kmem.lock);
  return i;
}
