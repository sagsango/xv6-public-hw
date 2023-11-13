#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void syscall_trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->signal_pending = 0;
	p->alarm_timer = 0;
	memset(p->signal_handler, 0, sizeof(p->signal_handler));
	memset(p->should_ignore_signal, 0, sizeof(p->should_ignore_signal));
	p->is_fgproc = 0;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= sizeof(addr_t);
  *(addr_t*)sp = (addr_t)syscall_trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->rip = (addr_t)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");

  inituvm(p->pgdir, _binary_initcode_start,
          (addr_t)_binary_initcode_size);
  p->sz = PGSIZE * 2;
  memset(p->tf, 0, sizeof(*p->tf));

  p->tf->r11 = FL_IF;  // with SYSRET, EFLAGS is in R11
  p->tf->rsp = p->sz;
  p->tf->rcx = PGSIZE;  // with SYSRET, RIP is in RCX

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  __sync_synchronize();
  p->state = RUNNABLE;
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int64 n)
{
  addr_t sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}
//PAGEBREAK!

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %rax so that fork returns 0 in the child.
  np->tf->rax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  __sync_synchronize();
  np->state = RUNNABLE;

  return pid;
}

//PAGEBREAK!
// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

//PAGEBREAK!
// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  int i = 0;
  struct proc *p;
  int skipped = 0;
  for(;;){
    ++i;
    // Enable interrupts on this processor.
    sti();
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE) {
        skipped++;
        continue;
      }
      skipped = 0;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    if (skipped > NPROC) {
      hlt();
      skipped = 0;
    }
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;


  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

//PAGEBREAK!
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

/*
int from_bcd(int code) {
  return (code>>4)*10+(code&0xf);
}
*/

/*
int get_cur_time_in_sec(){
      outb(0x70, 0x00);  // Request the seconds
      int secs = from_bcd(inb(0x71));
      outb(0x70, 0x02);  // Request the mins
      int mins = from_bcd(inb(0x71));
      outb(0x70, 0x04);  // Request the hours
      int hours = from_bcd(inb(0x71));
			return hours * 60 * 60 + mins * 60 + secs;
}
*/

void
alarm(int secs) {
	cprintf("alarm(): Updated alarm timer\n");
	acquire(&ptable.lock);
	proc->alarm_timer = get_cur_time_in_sec() + secs;
	release(&ptable.lock);
  cprintf("alarm(): Updated alarm timer\n");
}

void
signal(int signum, void (*handler)(int)) {
	cprintf("signal(): Updated signal handler, with number %d and handler %x\n",signum,(int)handler);
	acquire(&ptable.lock);
	switch((int)handler) {
		case 0:
			proc->should_ignore_signal[signum] = 0; // do not ignore
			proc->signal_handler[signum] = 0;
			break;
		case 1:
			proc->should_ignore_signal[signum] = 1; // ignore
			proc->signal_handler[signum] = 0;
			break;
		default:
			proc->signal_handler[signum] = handler;
			proc->should_ignore_signal[signum] = 0;
	}
	release(&ptable.lock);
  cprintf("signal(): Updated signal handler, with number %d and handler %x\n",signum,(int)handler);
}

void
check_signals(int x) {
  if (proc->killed)
    exit();

	acquire(&ptable.lock);
  if(proc && proc->signal_pending) {
	cprintf("check_signal(): signal_pending:%d, signal_ignore:%d, signal_handler:%p\n",
			proc->signal_pending, proc->should_ignore_signal[proc->signal_pending], proc->signal_handler[proc->signal_pending]);
		if (!proc->should_ignore_signal[proc->signal_pending]) {
			if (!proc->signal_handler[proc->signal_pending]) {
				release(&ptable.lock);
    		cprintf("check_signal(): got a signal but no signal-handler, exiting\n");
				exit();
				panic("check_signals(): should be killed");
			}
			else {
				// call signal handler


				memmove(&proc->saved_tf, proc->tf, sizeof(struct trapframe));
				// 1. code of sigret
  			proc->tf->rsp -= (uint)&sigret_end - (uint)&sigret_start;
  			memmove((void*)proc->tf->rsp, sigret_start, (uint)&sigret_end - (uint)&sigret_start);

				int a = sizeof(proc->signal_pending);
				int b = sizeof(proc->tf->rsp);

				// 2. argumnet for sig_handler
  			*((int*)(proc->tf->rsp - a)) = proc->signal_pending;
				// 3. pointer to the code of segret
  			*((int*)(proc->tf->rsp - a - b)) = proc->tf->rsp;
  			proc->tf->rsp -= a + b;

				/*
				proc->tf->rsp -= sizeof(proc->signal_pending);
				*((int*)(proc->tf->rsp)) = proc->signal_pending; // arg
				proc->tf->rsp -= sizeof(proc->tf->rsp);
				*((int*)(proc->tf->rsp)) = proc->tf->rsp;        // 
				*/

				// 4. rip points to signal handler.
  			//proc->tf->rip = (uint)proc->signal_handler[proc->signal_pending];
				proc->tf->rcx = (uint)proc->signal_handler[proc->signal_pending];

				cprintf("check_signal(): got a signal, and i am going to call handler: %p = %p\n", proc->tf->rip, proc->signal_handler[proc->signal_pending]);


				// 5. make pending signal = 0
				proc->signal_pending = 0;
			}
		}
  }
end:
		release(&ptable.lock);
	
}

// Signal the process with the given pid and signal
int
kill(int pid, int signal)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) { 
    if(p->pid == pid){
      p->killed = 1;
      
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      
      return 0;
    }
  }
  release(&ptable.lock);
  
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  addr_t pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getstackpcs((addr_t*)p->context->rbp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int from_bcd(int code) {
  return (code>>4)*10+(code&0xf);
}

int
get_cur_time_in_sec() {
      outb(0x70, 0x00);  // Request the seconds
      int secs = from_bcd(inb(0x71));
      outb(0x70, 0x02);  // Request the mins
      int mins = from_bcd(inb(0x71));
      outb(0x70, 0x04);  // Request the hours
      int hours = from_bcd(inb(0x71));

      return hours * 60 * 60 + mins * 60 + secs;
}

void update_alarm_signal() {
	//cprintf("update_alarm_signal()");
  struct proc *p;

  acquire(&ptable.lock);

  int cur_time_sec = get_cur_time_in_sec();

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->state == UNUSED ||
				p->state == EMBRYO ||
				p->state == ZOMBIE) {
      continue;
    }
    if (p->alarm_timer &&
        p->alarm_timer < cur_time_sec) {
      p->alarm_timer = 0;
      p->signal_pending = SIGALRM;
			if (p->state == SLEEPING) {
				p->state = RUNNABLE;
			}
    }
  }

  release(&ptable.lock);
}

int sigret(void) {
		cprintf("In my sigret\n");
		acquire(&ptable.lock);
		cprintf("Restoring tf\n");
		memmove(proc->tf, &proc->saved_tf, sizeof(struct trapframe));
		cprintf("Restored tf\n");
		release(&ptable.lock);
		cprintf("Out my sigret\n");
		return 0;
}


void
fgproc(int pid) {
	struct proc *p;
	acquire(&ptable.lock);
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		p->is_fgproc = 0; /* Unmark previous gfproc */
		if (p->pid == pid) {
				p->is_fgproc = 1;
				cprintf("makeing [%d] as fgproc\n", pid);
		}
	}
	release(&ptable.lock);
}

void
killfg() {
	struct proc *p;
	cprintf("In killfg\n");
  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		cprintf("Checking fgproc\n");
    if (p->is_fgproc) {
			p->killed = 1;

			cprintf("Killing %d ctr+c\n", p->pid);
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;

      break;
    }
  }
  release(&ptable.lock);
}

