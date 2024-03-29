#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "uproc.h"

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

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
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
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      return -1;
    }
    sleep(&ticks, (struct spinlock *)0);
  }
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  xticks = ticks;
  return xticks;
}

//Turn of the computer
int
sys_halt(void){
  cprintf("Shutting down ...\n");
  outw( 0x604, 0x0 | 0x2000);
  return 0;
}

#ifdef CS333_P1
//Return the current date
int
sys_date(void)
{
  struct rtcdate *d;

  if(argptr(0, (void*)&d, sizeof(struct rtcdate)) < 0)
    return -1;
  cmostime(d);
    return 0;
}
#endif
#ifdef  CS333_P2
uint
sys_getuid(void)
{
  return proc->uid;
}

uint
sys_getgid(void)
{
  return proc->gid;
}

uint
sys_getppid(void)
{
  if(proc->parent)
    return proc->parent->pid;
  else
    return proc->pid;
}

int
sys_setuid(void)
{
  int uid = 0;
  if(argint(0, &uid) < 0)
    return -1;

  if((uid < 0) || (uid > 32767))
    return -1;

  proc->uid = uid;
    return 0;
}

int
sys_setgid(void)
{
  int gid = 0;
  if(argint(0, &gid) < 0)
    return -1;

  if((gid < 0) || (gid > 32767))
    return -1;

  proc->gid = gid;
    return 0;
}

int
sys_getprocs(void)
{
  int max = 0;
  struct uproc *table;

  if(argint(0, &max) < 0)
    return -1;
  if(argptr(1, (void*)&table, sizeof(&table) * max) < 0)
    return -1;

  return getprocs(max, table);
}
#endif
#ifdef CS333_P3P4
int
sys_setpriority(void)
{
  int pID  = 0;
  int prio = 0;

  if(argint(0, &pID) < 0)
    return -1;
  if(argint(1, &prio) < 0)
    return -1;

  if(pID < 1)
    return -1;
  if(prio < 0 || prio > MAXPRIO)
    return -1;

  return setpriority(pID, prio);
}
#endif
