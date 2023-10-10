#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

addr_t
sys_sbrk(void)
{
  addr_t addr;
  addr_t n;

  argaddr(0, &n);
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

#define MMAP_FAILED ((~0lu))
#define NMMAP				10
#define MMAP_EAGER 	0
#define MMAP_LAZY 	1

  addr_t
sys_mmap(void)
{
  int fd, flags;
  if(argint(0,&fd) < 0 || argint(1,&flags) < 0)
    return MMAP_FAILED;

	struct file *f;
	char *old_mmaptop, *new_mmaptop, *t;
	int size, i, map_idx;

	/* TODO: check if agr fd valid*/
	f = proc->ofile[fd];
	if (!f || !f->readable)
		return MMAP_FAILED;

	map_idx = -1;
	for (i = 0; i < NMMAP; ++i)
	{
		if (proc->mmaps[i].fd == -1)
		{
			map_idx = i;
			break;
		}
	}

	if (map_idx == -1)
		return MMAP_FAILED;

	size = f->ip->size;
	old_mmaptop = proc->mmaptop;
	new_mmaptop = PGROUNDUP((long unsigned int)old_mmaptop + size);

	proc->mmaptop = new_mmaptop;
	proc->mmaps[map_idx].fd = fd;
	proc->mmaps[map_idx].start = old_mmaptop;

	switch(flags)
	{
		case MMAP_EAGER:
			/* XXX: To load file into memory we could have used functions used in exec */
			if ((t = allocuvm(proc->pgdir, old_mmaptop, new_mmaptop)) ==0)
				return MMAP_FAILED;
			if (t != new_mmaptop)
				return MMAP_FAILED;
			if ((fileread(f, old_mmaptop, size)) != size)
				return MMAP_FAILED;
			/* flush the cache */
			switchuvm(proc);
			break;

		case MMAP_LAZY:
			break;

		default:
			return MMAP_FAILED;
	}

	return old_mmaptop;
}

  int
handle_pagefault(addr_t va)
{
	struct file *f;
	int i, fd, size;
	char * t, *va_start, *va_end;

	for (i = 0; i < NMMAP; ++i)
	{
		if (proc->mmaps[i].fd == -1)
			continue;

		fd = proc->mmaps[i].fd;
		f = proc->ofile[fd];
		size = f->ip->size;

		if ((va <= PGROUNDUP(proc->mmaps[i].start + size)))
		{
			/* XXX: To load file into memory we could have used functions used in exec */
			va_start = PGROUNDDOWN(va);
			va_end = va_start + PGSIZE;

			if ((t = allocuvm(proc->pgdir, va_start, va_end)) ==0){
				return 0;
			}

			f->off = (int)(va_start - proc->mmaps[i].start);

			if ((fileread(f, va_start, PGSIZE)) <= 0){
				return 0;
			}

			/* flush the cache */
			switchuvm(proc);

			return 1; /* sucess */
		}
	}
	return 0; /* fail */
}
