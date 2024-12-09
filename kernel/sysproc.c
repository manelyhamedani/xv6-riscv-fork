#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "trap.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_child_processes(void) {
  struct child_processes *cp;
  struct child_processes kcp;
  argaddr(0, (uint64 *) &cp);
  struct proc *p = myproc();
  copyin(p->pagetable, (char *) &kcp, (uint64) cp, sizeof(kcp));
  int xstat = child_processes(&kcp);
  copyout(p->pagetable, (uint64) cp, (char *) &kcp, sizeof(kcp));
  return xstat;
}

uint64 sys_myrep(void) {
  struct report_traps *rt;
  struct report_traps krt;
  argaddr(0, (uint64 *) &rt);
  struct proc *p = myproc();
  copyin(p->pagetable, (char *) &krt, (uint64) rt, sizeof(krt));
  int xstat = myrep(&krt);
  copyout(p->pagetable, (uint64) rt, (char *) &krt, sizeof(krt));
  return xstat;
}

uint64 sys_sysrep(void) {
  struct report_traps *rt;
  struct report_traps krt;
  argaddr(0, (uint64 *) &rt);
  struct proc *p = myproc();
  copyin(p->pagetable, (char *) &krt, (uint64) rt, sizeof(krt));
  int xstat = sysrep(&krt);
  copyout(p->pagetable, (uint64) rt, (char *) &krt, sizeof(krt));
  return xstat;
}

uint64 sys_create_thread(void) {
  void *(*runner)(void *);
  void *arg;
  void *stack;
  
  argaddr(0, (uint64 *) &runner);
  argaddr(1, (uint64 *) &arg);
  argaddr(2, (uint64 *) &stack);  

  return create_thread(runner, (void *) arg, &stack);
}

uint64 sys_join_thread(void) {
  int tid;

  argint(0, &tid);

  return join_thread(tid);
}

uint64 sys_stop_thread(void) {
  int tid;

  argint(0, &tid);

  return stop_thread(tid);
}