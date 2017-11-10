// Layout of the trap frame built on the stack by the
// hardware and by trapasm.S, and passed to trap().
struct trapframe {
   uint64 rax;      
   uint64 rbx;
   uint64 rcx;
   uint64 rdx;
   uint64 rbp;
   uint64 rsi;
   uint64 rdi;
   uint64 r8;
   uint64 r9;
   uint64 r10;
   uint64 r11;
   uint64 r12;
   uint64 r13;
   uint64 r14;
   uint64 r15;
 
   uint64 trapno;
   uint64 err;
 
   uint64 rip;     // for syscall, this is ignored. use rcx
   uint64 cs;      // for syscall, this is also ignored, use MSR_CSTAR
   uint64 rflags;  
   uint64 rsp;     
   uint64 ss;      
};

// Per-CPU state
struct cpu {
  uchar id;
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?

  // Cpu-local storage variables; see below
  void *local;
  //struct cpu *cpu;
  //struct proc *proc;           // The currently-running process.
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Per-CPU variables, holding pointers to the
// current cpu and to the current process.
// The asm suffix tells gcc to use "%gs:0" to refer to cpu
// and "%gs:4" to refer to proc.  seginit sets up the
// %gs segment register so that %gs refers to the memory
// holding those two variables in the local cpu's struct cpu.
// This is similar to how thread-local variables are implemented
// in thread libraries such as Linux pthreads.
extern __thread struct cpu *cpu;
extern __thread struct proc *proc;

//extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
//extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  addr_t r15;
  addr_t r14;
  addr_t r13;
  addr_t r12;
  addr_t r11;
  addr_t rbx;
  addr_t ebp; //rbp
  addr_t eip; //rip;

};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };
enum signal { SIGINT=2, SIGKILL=9, SIGALRM=14 };

// Per-process state
struct proc {
  addr_t sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)

  enum signal signal_pending;  
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
