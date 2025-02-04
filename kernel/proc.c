#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include <stddef.h>
#include "trap.h"
#include "proc.h"
#include "stat.h"
#include "sleeplock.h"
#include "fs.h"
#include "kfile.h"

#define INF 0xFFFFFFFF 

enum scheduler sched_type = MinCU;

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct spinlock proc_lock;

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

int nexttid = 1;
struct spinlock tid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);
void min_cpu_usage_scheduler(void);


extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&tid_lock, "nexttid");
  initlock(&wait_lock, "wait_lock");
  initlock(&proc_lock, "proc_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid()
{
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

int alloctid() {
  int tid;

  acquire(&tid_lock);
  tid = nexttid;
  nexttid++;
  release(&tid_lock);

  return tid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;
  p->last_running_thread_index = 0;
  p->running_threads_count = 0;
  p->cpu_usage.start_tick = (uint) -1;
  p->cpu_usage.sum_of_ticks = 0;
  p->cpu_usage.quota = 0;
  p->deadline = (uint) -1;

  memset(p->threads, 0, sizeof(p->threads));
  
  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
struct proc *fork_core(void) {
  int i;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return NULL;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return NULL;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return np;
}

// simple fork.
int
fork(void)
{
  struct proc *np = fork_core();

  if (np) {
    return np->pid;
  }

  return -1;
}


// fork a process and set deadline for new process.
int deadfork(uint ttl) {
  struct proc *np = fork_core();

  if (np) {
    np->deadline = ticks + ttl;
    return np->pid;
  }

  return -1;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if(pp->state == ZOMBIE){
          // Found one.
          pid = pp->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                  sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || killed(p)){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Round Robin Scheduler
void
rr_scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  printf("RR Scheduler\n");

  c->proc = 0;
  c->thread = 0;
  for(;;){
    // The most recent process to run may have had interrupts
    // turned off; enable them to avoid a deadlock if all
    // processes are waiting.
    intr_on();

    if (sched_type == MinCU) {
      min_cpu_usage_scheduler();
    }

    int found = 0;
    int temp_found = 0;
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      struct trapframe *tf;
      struct trapframe ptf;
      if (p->state == RUNNABLE) {
        // just main thread
        if (p->threads[0].state == THREAD_FREE) {
          tf = p->trapframe;
          temp_found = 1;
        }
        else {
          int next_thread = p->last_running_thread_index;
          struct thread *t;
          
          for (int i = 0; i < MAX_THREAD; ++i) {
            next_thread++;

            if (next_thread >= MAX_THREAD) {
              next_thread = 0;
            }
            
            t = &p->threads[next_thread];

            if (t->state == THREAD_RUNNABLE) {
              t->state = THREAD_RUNNING;
              t->cpu = mycpu();
              c->thread = t;

              p->last_running_thread_index = next_thread;

              tf = t->trapframe;
              temp_found = 1;
              break;
            }
          }
        }
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        if (temp_found) {
          p->state = RUNNING;
          p->running_threads_count++;
          c->proc = p;    
          if (tf != p->trapframe) {
            ptf = *(p->trapframe);
            *(p->trapframe) = *tf;
          }
          // uvmunmap(p->pagetable, TRAPFRAME, 1, 0);
          // if(mappages(p->pagetable, TRAPFRAME, PGSIZE,
          //   (uint64)(tf), PTE_R | PTE_W) < 0){

          //   printf("trapframe mapping failed\n");
          p->cpu_usage.start_tick = ticks;
          swtch(&c->context, &p->context);
          p->cpu_usage.sum_of_ticks += ticks - p->cpu_usage.start_tick;

          if (tf != p->trapframe) {
            *(p->trapframe) = ptf;
          }
          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
          c->thread = 0;
          found = 1;
        
        }
      }
      temp_found = 0;
      release(&p->lock);
    }
    if(found == 0) {
      // nothing to run; stop running on this core until an interrupt.
      intr_on();
      asm volatile("wfi");
    }
  }
}

// Scheduler based on minimum cpu usage
void
min_cpu_usage_scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  int quota_enable = 1;

  printf("MinCU Scheduler\n");

  c->proc = 0;
  c->thread = 0;
  for(;;){
    // The most recent process to run may have had interrupts
    // turned off; enable them to avoid a deadlock if all
    // processes are waiting.
    intr_on();

    if (sched_type == RR) {
      rr_scheduler();
    }

    uint min_cu = INF;
    struct proc *min_cu_proc = NULL;
    int found = 0;

    acquire(&proc_lock);
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if (p->state == RUNNABLE && p->cpu_usage.sum_of_ticks < min_cu) {
        if (p->cpu_usage.quota <= p->cpu_usage.sum_of_ticks) {
          if (quota_enable) {
            release(&p->lock);
            continue;
          }
        }
        else {
          quota_enable = 1;
        }
        if (min_cu_proc) {
          release(&min_cu_proc->lock);
        }
        min_cu = p->cpu_usage.sum_of_ticks;
        min_cu_proc = p;
      }
      else if (p->state == RUNNABLE && p->cpu_usage.sum_of_ticks == min_cu) {
        if (min_cu_proc && p->deadline < min_cu_proc->deadline) {
          release(&min_cu_proc->lock);
          min_cu_proc = p;
        }
        else {
          release(&p->lock);
        }
      }
      else {
        release(&p->lock);
      }
    }
    release(&proc_lock);

    if (min_cu_proc) {
      p = min_cu_proc;

      int temp_found = 0;
      struct trapframe *tf;
      struct trapframe ptf;

      // just main thread
      if (p->threads[0].state == THREAD_FREE) {
        tf = p->trapframe;
        temp_found = 1;
      }
      else {
        int next_thread = p->last_running_thread_index;
        struct thread *t;
        
        for (int i = 0; i < MAX_THREAD; ++i) {
          next_thread++;

          if (next_thread >= MAX_THREAD) {
            next_thread = 0;
          }
          
          t = &p->threads[next_thread];

          if (t->state == THREAD_RUNNABLE) {
            t->state = THREAD_RUNNING;
            t->cpu = mycpu();
            c->thread = t;

            p->last_running_thread_index = next_thread;

            tf = t->trapframe;
            temp_found = 1;
            break;
          }
        }
      }
      // Switch to chosen process.  It is the process's job
      // to release its lock and then reacquire it
      // before jumping back to us.
      if (temp_found) {
        p->state = RUNNING;
        p->running_threads_count++;
        c->proc = p;    
        if (tf != p->trapframe) {
          ptf = *(p->trapframe);
          *(p->trapframe) = *tf;
        }

        p->cpu_usage.start_tick = ticks;
        swtch(&c->context, &p->context);
        p->cpu_usage.sum_of_ticks += ticks - p->cpu_usage.start_tick;
        
        if (tf != p->trapframe) {
          *(p->trapframe) = ptf;
        }
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
        c->thread = 0;
        found = 1;
      
      }
      temp_found = 0;
      release(&p->lock);
    }

    if(found == 0) {
      // nothing to run; stop running on this core until an interrupt.
      quota_enable = 0;
      intr_on();
      asm volatile("wfi");
    }
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void) {
  if (sched_type == RR)
    rr_scheduler();
  else if (sched_type == MinCU)
    min_cpu_usage_scheduler();
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();
  struct thread *t;

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING && p->running_threads_count == 0)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;

  if ((t = mycpu()->thread) != NULL && t->state != THREAD_FREE) {
    if (t->trapframe != p->trapframe) {
      *(t->trapframe) = *(p->trapframe);
    }
    if (t->state == THREAD_RUNNING) {
      t->state = THREAD_RUNNABLE;
    }
    t->cpu = NULL;
  }
  p->running_threads_count--;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    fsinit(ROOTDEV);

    first = 0;
    // ensure other cores see first=0.
    __sync_synchronize();
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

void drop_dead_processes(uint deadline) {
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state != UNUSED && p->state != ZOMBIE) {
      if (p->deadline != (uint) -1 && p->deadline <= deadline) {
        p->killed = 1;
        if(p->state == SLEEPING){
          // Wake process from sleep().
          p->state = RUNNABLE;
        }
      }
    }
    release(&p->lock);
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void
setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int
killed(struct proc *p)
{
  int k;
  
  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [USED]      "used",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

int child_processes(struct child_processes *cp) {
  struct proc *current_process = myproc();
  struct proc *p, *parent;
  int index = 0;

  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->state == UNUSED) {
      continue;
    }
    parent = p->parent;

    while (parent != NULL) {
      if (parent == current_process) {
        cp->count = cp->count + 1;
        strncpy(cp->processes[index].name, p->name, sizeof(p->name));
        cp->processes[index].pid = p->pid;
        cp->processes[index].ppid = p->parent->pid;
        cp->processes[index].state = p->state;
        ++index;
        break;
      }
      parent = parent->parent;
    }
  }

  return 0;
}

int myrep(struct report_traps *rt) {
  struct proc *current_process = myproc();

  acquire(&_internal_report_list.lock);

  // read reports according to time
  int report_count = _internal_report_list.count;
  int read_index = (report_count <= REPORT_BUFFER_SIZE) ? 0 : _internal_report_list.write_index;

  for (int i = 0; (i < REPORT_BUFFER_SIZE && i < report_count); ++i) {
    int anc_count = _internal_report_list.reports[read_index].ancestors_count;

    for (int j = 0; j < anc_count; ++j) {

      if (_internal_report_list.reports[read_index].ancestors[j] == current_process->pid) {
        rt->reports[rt->count++] = _internal_report_list.reports[read_index];
        break;
      }
      
    }

    read_index = (read_index + 1) % REPORT_BUFFER_SIZE;
  }
  release(&_internal_report_list.lock);
  return 0;
}

int sysrep(struct report_traps *rt) {
  struct file *f = kfilealloc();
  f->type = FD_INODE;
  // f->ip = namei("/reports.bin");
  // printf("size = %d\n", f->ip->size);
  if (f->ip == NULL) {
    begin_op();
    f->ip = kfilecreate("/reports.bin", T_FILE, 0, 0);
    // f->ip->size = 0;
    iunlockput(f->ip);
    end_op();
    return 0;
  }
  // printf("size = %d\n", f->ip->size);
  f->off = 0;
  f->readable = 1;
  f->ref = 1;
  
  kfileseek(f, 0, SEEK_SET);
  kfileread(f, (uint64) &(rt->count), sizeof(int));
  kfileseek(f, -10 * (int) sizeof(struct report), SEEK_END);
  for (int i = 0; i < REPORT_BUFFER_SIZE && i < rt->count; ++i) {
    kfileread(f, (uint64) &rt->reports[i], sizeof(struct report));
  }
  kfileclose(f);
  return 0;
}

int create_thread(void *(*runner)(void *), void *arg, void *stack) {
  struct proc *p = myproc();
  int tid = -1;
  
  acquire(&p->lock);

  if (p->threads[0].state == THREAD_FREE) {
    // add main thread
    p->threads[0].state = THREAD_RUNNING;
    p->threads[0].id = alloctid();
    p->threads[0].join = 0;
    p->threads[0].trapframe = p->trapframe;
    p->threads[0].cpu = mycpu();
    mycpu()->thread = &p->threads[0];
  }

  for (int i = 0; i < MAX_THREAD; ++i) {

    if (p->threads[i].state == THREAD_FREE) {
      p->threads[i].state = THREAD_RUNNABLE;
      p->threads[i].id = alloctid();
      p->threads[i].join = 0;
      p->threads[i].trapframe = (struct trapframe *) kalloc();
      p->threads[i].cpu = NULL;

      // initialize trapframe
      memset(p->threads[i].trapframe, 0, sizeof(struct trapframe));
      p->threads[i].trapframe->epc = (uint64) runner;
      p->threads[i].trapframe->sp = (uint64) stack;  // point to the top of stack
      p->threads[i].trapframe->a0 = (uint64) arg;
      p->threads[i].trapframe->ra = (uint64) -1;

      tid = p->threads[i].id;

      break;
    }

  }

  release(&p->lock);
  return tid;

}

int join_thread(int tid) {
  struct proc *p = myproc();
  struct thread *t;

  acquire(&p->lock);

  for (int i = 0; i < MAX_THREAD; ++i) {
    if (p->threads[i].id == tid && p->threads[i].state != THREAD_FREE) {
      t = mycpu()->thread;

      t->join = tid;
      t->state = THREAD_JOINED;

      release(&p->lock);
      yield();
      usertrapret();
      return 0;
    }
  }

  // no thread exists with the specified thread ID (tid)
  release(&p->lock);
  return -1;
}

int thread_cleanup(int tid) {
  struct proc *p = myproc();
  struct thread *t;

  acquire(&p->lock);

  for (t = &p->threads[0]; t < &p->threads[MAX_THREAD]; ++t) {
    if (t->id == tid) {

      if (t->state != THREAD_FREE) {
        t->cpu = NULL;
        t->state = THREAD_FREE;
        kfree(t->trapframe);

        // main thread cleaning up terminates the whole process
        if (t->id == 1) {
          for (int i = 0; i < MAX_THREAD; ++i) {
            if (p->threads[i].state != THREAD_FREE) {
              kfree(p->threads[i].trapframe);
            }
          }
          release(&p->lock);
          exit(0);
          return 0;
        }

        // wakeup joined threads
        for (int i = 0; i < MAX_THREAD; ++i) {
          if (p->threads[i].state == THREAD_JOINED && p->threads[i].join == t->id) {
              p->threads[i].join = 0;
              p->threads[i].state = THREAD_RUNNABLE;
          }
        }
      }

      release(&p->lock);
      yield();
      usertrapret();
      return 0;
    }
  }

  release(&p->lock);
  return -1;
}

int stop_thread(int tid) {
  struct thread *t;

  // stop running thread
  if (tid == -1) {
    if ((t = mycpu()->thread) != NULL) {
      thread_cleanup(t->id);
    }
    return 0;
  }

  return thread_cleanup(tid);
}

int cpu_usage() {
  struct proc *p = myproc();
  uint64 total_cpu_usage;

  acquire(&p->lock);
  acquire(&tickslock);
  total_cpu_usage = p->cpu_usage.sum_of_ticks;
  release(&tickslock);
  release(&p->lock);

  return total_cpu_usage;
}

int top(struct top *t) {
  struct proc *p;
  int index = 0;
  t->count = 0;

  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->state == UNUSED) {
      release(&p->lock);
      continue;
    }
    
    t->count += 1;

    int i;
    for (i = index - 1; i >= 0; --i) {
      if (t->processes[i].cpu_usage.sum_of_ticks <= p->cpu_usage.sum_of_ticks) {
        t->processes[i + 1] = t->processes[i];
      }
      else {
        break;
      }
    }

    i += 1;
    strncpy(t->processes[i].name, p->name, sizeof(p->name));
    t->processes[i].pid = p->pid;
    if (p->parent == NULL) {
      t->processes[i].ppid = 0;
    }
    else {
      t->processes[i].ppid = p->parent->pid;
    }
    t->processes[i].state = p->state;
    t->processes[i].cpu_usage = p->cpu_usage;

    index += 1;
    release(&p->lock);
  }


  return 0;
}


int set_cpu_quota(int pid, uint quota) {
  struct proc *current_process = myproc();
  struct proc *p, *parent, *target_proc = NULL;


  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->pid == pid) {
      target_proc = p;
      break;
    }
  }

  if (target_proc == NULL) {
    // process not found.
    return -1;
  }

  parent = target_proc;

  while (parent != NULL) {
    if (parent != current_process) {
      parent = target_proc->parent;
    }
    else {
      break;
    }
  }

  if (parent == NULL) {
    // process with pid is not in current procces subtree.
    return -1;
  }

  target_proc->cpu_usage.quota = quota;
  return 0;
}