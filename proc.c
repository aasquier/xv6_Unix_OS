#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#ifdef CS333_P2
#include "uproc.h"
#endif

#ifdef CS333_P3P4
struct StateLists {
  struct proc* ready[MAXPRIO+1];
  struct proc* readyTail[MAXPRIO+1];
  struct proc* free;
  struct proc* freeTail;
  struct proc* sleep;
  struct proc* sleepTail;
  struct proc* zombie;
  struct proc* zombieTail;
  struct proc* running;
  struct proc* runningTail;
  struct proc* embryo;
  struct proc* embryoTail;
};
#endif

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  #ifdef CS333_P3P4
  struct StateLists pLists;
  uint PromoteAtTime;
  #endif
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);


#ifdef CS333_P3P4
static void initProcessLists(void);
static void initFreeList(void);
static int  stateListAdd(struct proc** head, struct proc** tail, struct proc* p);
static int  stateListRemove(struct proc** head, struct proc** tail, struct proc* p);

static int  exitSearchAll(struct proc** head, struct proc* pExit);
static int  killSearchAll(struct proc** head, int pid);
static int  waitSearchAll(struct proc** head, int* havekids, struct proc* pWait);
static void changeState(struct proc** headRemove, struct proc** tailRemove, struct proc** headAdd, struct proc** tailAdd, struct proc* p, enum procstate stateRemove, enum procstate stateAdd, char* err);
static void assertState(struct proc* p, enum procstate state);

static int  exitSearchReady(struct proc* p);
static int  killSearchReady(int pid);
static int  waitSearchReady(int * havekids, struct proc* p);
static void promoteProcs(void);
static void assertPrio(struct proc* p, uint prio);
#endif

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

#ifndef CS333_P3P4
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
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  #ifdef CS333_P1
  p->start_ticks = ticks;
  #endif

  #ifdef CS333_P2
  p->cpu_ticks_total = 0;
  p->cpu_ticks_in    = 0;
  #endif

  return p;
}
#else
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  p = ptable.pLists.free;
  if(p){
    changeState(&ptable.pLists.free, &ptable.pLists.freeTail, &ptable.pLists.embryo, &ptable.pLists.embryoTail, p, UNUSED, EMBRYO, "allocproc()");
    goto found;
  }
  release(&ptable.lock);
  return 0;

found:
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    acquire(&ptable.lock);
    changeState(&ptable.pLists.embryo, &ptable.pLists.embryoTail, &ptable.pLists.free, &ptable.pLists.freeTail, p, EMBRYO, UNUSED, "allocproc()");
    release(&ptable.lock);

    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  #ifdef CS333_P1
  p->start_ticks = ticks;
  #endif

  #ifdef CS333_P2
  p->cpu_ticks_total = 0;
  p->cpu_ticks_in    = 0;
  #endif

  p->priority = 0;
  p->budget   = MAX_BUDGET;

  return p;
}
#endif

#ifdef CS333_P2
// Function to copy pertinent info to a user version of the ptable (utable)
int
getprocs(uint max, struct uproc* utable)
{
  struct proc *p = ptable.proc;
  uint i = 0;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC] && i < max; p++){
    if(p->state != UNUSED && p->state != EMBRYO){
      utable[i].pid             = p->pid;
      utable[i].uid             = p->uid;
      utable[i].gid             = p->gid;
      utable[i].elapsed_ticks   = ticks - p->start_ticks;
      utable[i].CPU_total_ticks = p->cpu_ticks_total;
      utable[i].size            = p->sz;
      #ifdef CS333_P3P4
      utable[i].priority        = p->priority;
      #endif
      safestrcpy(utable[i].name, p->name, sizeof(char) * STRMAX);

      if(p->parent)
        utable[i].ppid = p->parent->pid;
      else
        utable[i].ppid = p->pid;

      switch(p->state){
        case UNUSED:   safestrcpy(utable[i].state, "unused", STRMAX);   break;
        case EMBRYO:   safestrcpy(utable[i].state, "embryo", STRMAX);   break;
        case SLEEPING: safestrcpy(utable[i].state, "sleep", STRMAX);    break;
        case RUNNABLE: safestrcpy(utable[i].state, "runble", STRMAX);   break;
        case RUNNING:  safestrcpy(utable[i].state, "run", STRMAX);      break;
        case ZOMBIE:   safestrcpy(utable[i].state, "zombie", STRMAX);   break;
        default:                                                        break;
      }

      i++;
    }
  }
  release(&ptable.lock);

  if(i < 1)    // If no procs were copied we have an error because at least the init and sh procs should be running
    return -1;
  else
    return i;   // Return nummber of processes that were actally copied

}
#endif

#ifdef CS333_P3P4
int
setpriority(uint pID, uint prio)
{
  int i;
  int rc;
  struct proc* p;

  acquire(&ptable.lock);

  for(i=0; i < MAXPRIO+1; i++){
    for(p = ptable.pLists.ready[i]; p != 0; p = p->next){
      if(p->pid == pID){
        if(p->priority != prio){
          rc = stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.readyTail[i], p);
          if(rc == -1)
            panic("Failed to remove process from the proper ready list. setpriority()");
          assertState(p, RUNNABLE);
          assertPrio(p, i);
          p->priority = prio;
          p->budget = MAX_BUDGET;
          rc = stateListAdd(&ptable.pLists.ready[p->priority], &ptable.pLists.readyTail[p->priority], p);
          if(rc == -1)
            panic("Failed to add process to the proper ready list. setpriority()");
        }
        release(&ptable.lock);
        return 0;
      }
    }
  }
  for(p = ptable.pLists.sleep; p != 0; p = p->next){
    if(p->pid == pID){
      if(p->priority != prio){
        assertState(p, SLEEPING);
        p->priority = prio;
        p->budget = MAX_BUDGET;
      }
      release(&ptable.lock);
      return 0;
    }
  }
  for(p = ptable.pLists.running; p != 0; p = p->next){
    if(p->pid == pID){
      if(p->priority != prio){
        assertState(p, RUNNING);
        p->priority = prio;
        p->budget = MAX_BUDGET;
      }
      release(&ptable.lock);
      return 0;
    }
  }

  release(&ptable.lock);

  return -1;
}
#endif

#ifndef CS333_P3P4
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
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  #ifdef CS333_P2
  p->uid = INIT_UID;
  p->gid = INIT_GID;
  #endif

  p->state = RUNNABLE;
}
#else
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  int rc;

  acquire(&ptable.lock);
  initProcessLists();
  initFreeList();
  ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
  release(&ptable.lock);

  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  #ifdef CS333_P2
  p->uid = INIT_UID;
  p->gid = INIT_GID;
  #endif

  rc = stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryoTail, p);
  if(rc == -1)
    panic("Error: Process to be removed from EMBRYO list does not exist. (proc.c: userinit(): Line 313");               // Traversal of the ready list failed to find match, or ready list empty
  assertState(p, EMBRYO);
  p->state = RUNNABLE;

  ptable.pLists.ready[0]     = p;
  ptable.pLists.readyTail[0] = p;
  p->next                    = 0;
}
#endif

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

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

#ifndef CS333_P3P4
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

  #ifdef CS333_P2
  np->uid = proc->uid;
  np->gid = proc->gid;
  #endif

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);

  return pid;
}
#else
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

    acquire(&ptable.lock);
    changeState(&ptable.pLists.embryo, &ptable.pLists.embryoTail, &ptable.pLists.free, &ptable.pLists.freeTail, np, EMBRYO, UNUSED, "fork()");
    release(&ptable.lock);

    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  #ifdef CS333_P2
  np->uid = proc->uid;
  np->gid = proc->gid;
  #endif

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  changeState(&ptable.pLists.embryo, &ptable.pLists.embryoTail, &ptable.pLists.ready[0], &ptable.pLists.readyTail[0], np, EMBRYO, RUNNABLE, "fork()");
  release(&ptable.lock);

  return pid;
}
#endif

#ifndef CS333_P3P4
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
#else
void
exit(void)
{
  int found = 0;
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

  // Search all of the lists for the process
  found = exitSearchAll(&ptable.pLists.embryo, proc);
  if(!found)
    found = exitSearchReady(proc);
  if(!found)
    found = exitSearchAll(&ptable.pLists.sleep , proc);
  if(!found)
    found = exitSearchAll(&ptable.pLists.running , proc);
  if(!found)
    found = exitSearchAll(&ptable.pLists.zombie , proc);

  // Jump into the scheduler, never to return.
  changeState(&ptable.pLists.running, &ptable.pLists.runningTail, &ptable.pLists.zombie, &ptable.pLists.zombieTail, proc, RUNNING, ZOMBIE, "exit()");

  sched();
  panic("zombie exit");
}

// Helper function for exit()s search all
static int
exitSearchAll(struct proc** head, struct proc* pExit)
{
  struct proc* p;
  struct proc*  pZ;

  for(p = *head; p != 0; p = p->next){
    if(p->parent == pExit){
      p->parent = initproc;
      for(pZ = ptable.pLists.zombie; pZ != 0; pZ = pZ->next){
        if(p == pZ)
          wakeup1(initproc);
        return 1;
      }
    }
  }

  return 0;
}

static int
exitSearchReady(struct proc* pExit)
{
  struct proc* p;
  struct proc* pZ;
  int i;

  for(i=0; i < MAXPRIO+1; i++){
    for(p = ptable.pLists.ready[i]; p != 0; p = p->next){
      if(p->parent == pExit){
        p->parent = initproc;
        for(pZ = ptable.pLists.zombie; pZ != 0; pZ = pZ->next){
          if(p == pZ)
            wakeup1(initproc);
          return 1;
        }
      }
    }
  }

  return 0;

}
#endif

#ifndef CS333_P3P4
// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
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
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
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
#else
int
wait(void)
{
  int pid;
  int havekids;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;

    pid = waitSearchAll(&ptable.pLists.embryo, &havekids, proc);
    if(pid == -1)
      pid = waitSearchReady(&havekids, proc);
    if(pid == -1)
      pid = waitSearchAll(&ptable.pLists.sleep, &havekids, proc);
    if(pid == -1)
      pid = waitSearchAll(&ptable.pLists.running, &havekids, proc);
    if(pid == -1)
      pid = waitSearchAll(&ptable.pLists.zombie, &havekids, proc);
    if(pid != -1){
      release(&ptable.lock);
      return pid;
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

// wait()s searchAll helper
static int
waitSearchAll(struct proc** head, int* havekids, struct proc* pWait)
{
  int pid = -1;
  struct proc* p1;
  struct proc* p2;

  for(p1 = *head; p1 != 0 ; p1 = p1->next){
      if(p1->parent != pWait)
        continue;
      *havekids = 1;

      for(p2 = ptable.pLists.zombie; p2 != 0; p2 = p2->next){
        if(p1 == p2){
          pid = p1->pid;
          kfree(p1->kstack);
          p1->kstack = 0;
          freevm(p1->pgdir);

          changeState(&ptable.pLists.zombie, &ptable.pLists.zombieTail, &ptable.pLists.free, &ptable.pLists.freeTail, p1, ZOMBIE, UNUSED, "waitSearchAll()");

          p1->pid = 0;
          p1->parent = 0;
          p1->name[0] = 0;
          p1->killed = 0;
        }
      }
    }
  return pid;
}

static int
waitSearchReady(int* havekids, struct proc* pWait)
{
  int pid = -1;
  int i;
  struct proc* p1;
  struct proc* p2;

  for(i=0; i < MAXPRIO+1; i++){
    for(p1 = ptable.pLists.ready[i]; p1 != 0 ; p1 = p1->next){
        if(p1->parent != pWait)
          continue;
        *havekids = 1;

        for(p2 = ptable.pLists.zombie; p2 != 0; p2 = p2->next){
          if(p1 == p2){
            pid = p1->pid;
            kfree(p1->kstack);
            p1->kstack = 0;
            freevm(p1->pgdir);

            changeState(&ptable.pLists.zombie, &ptable.pLists.zombieTail, &ptable.pLists.free, &ptable.pLists.freeTail, p1, ZOMBIE, UNUSED, "waitSearchReady()");

            p1->pid = 0;
            p1->parent = 0;
            p1->name[0] = 0;
          p1->killed = 0;
        }
      }
    }
  }
  return pid;
}
#endif

#ifndef CS333_P3P4
//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
// original xv6 scheduler. Use if CS333_P3P4 NOT defined.
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);
      p->state = RUNNING;

      #ifdef CS333_P2
      p->cpu_ticks_in = ticks;
      #endif

      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}
#else
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle
  int i;
  int rc;


  for(;;){
    // Enable interrupts on this processor.
    sti();
    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.

    acquire(&ptable.lock);
    if(ticks >= ptable.PromoteAtTime){
      promoteProcs();
      ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
    }

    for(i=0; i < MAXPRIO+1; i++){
      // Remove process from the ready list and place on the running list
      p = ptable.pLists.ready[i];
      if(p){

        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        idle = 0;  // not idle this timeslice
        proc = p;
        switchuvm(p);

        rc = stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.readyTail[i],  p);
        if(rc == -1){
          panic("Process failed to be removed from the ready list. scheduler()");
        }
        assertState(p, RUNNABLE);
        assertPrio(p, i);

        p->state = RUNNING;

        rc = stateListAdd(&ptable.pLists.running, &ptable.pLists.runningTail, p);
        if(rc == -1){
          panic("Process failed to be added to the proper ready list. scheduler()");
        }

        #ifdef CS333_P2
        p->cpu_ticks_in = ticks;
        #endif
        swtch(&cpu->scheduler, proc->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        proc = 0;
        break;
      }
    }

    release(&ptable.lock);


    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}

static void
promoteProcs(void)
{
  int i;
  int rc;
  struct proc* p;

  for(i=1; i < MAXPRIO+1; i++){
    for(p = ptable.pLists.ready[i]; p != 0; p = ptable.pLists.ready[i]){
      rc = stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.readyTail[i], p);
      if(rc == -1)
        panic("Failed to remove process from the proper ready list. promoteProcs()");
      assertState(p, RUNNABLE);
      assertPrio(p, i);
      p->priority--;
      p->budget = MAX_BUDGET;
      rc = stateListAdd(&ptable.pLists.ready[i-1], &ptable.pLists.readyTail[i-1], p);
      if(rc == -1)
        panic("Failed to add process to the proper ready list. promoteProcs()");
    }
  }
  for(p = ptable.pLists.sleep; p != 0; p = p->next){
      assertState(p, SLEEPING);
      if(p->priority > 0)
        p->priority--;
      p->budget = MAX_BUDGET;
  }
  for(p = ptable.pLists.running; p != 0; p = p->next){
      assertState(p, RUNNING);
      if(p->priority > 0)
        p->priority--;
      p->budget = MAX_BUDGET;
  }
}
#endif
// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
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

  #ifdef CS333_P2
  proc->cpu_ticks_total += ticks - proc->cpu_ticks_in;
  #endif

  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

#ifndef CS333_P3P4
// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}
#else
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock

  proc->budget = proc->budget - (ticks - proc->cpu_ticks_in);
  if(proc->budget <= 0){
    proc->budget = MAX_BUDGET;
    if(proc->priority < MAXPRIO)
      proc->priority++;
  }
  changeState(&ptable.pLists.running, &ptable.pLists.runningTail, &ptable.pLists.ready[proc->priority], &ptable.pLists.readyTail[proc->priority], proc, RUNNING, RUNNABLE, "yield()");

  sched();
  release(&ptable.lock);
}
#endif

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

#ifndef CS333_P3P4
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// 2016/12/28: ticklock removed from xv6. sleep() changed to
// accept a NULL lock to accommodate.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){
    acquire(&ptable.lock);
    if (lk) release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;

  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}
#else
void
sleep(void *chan, struct spinlock *lk)
{

  if(proc == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){
    acquire(&ptable.lock);
    if (lk) release(lk);
  }

  // Go to sleep.
  proc->chan = chan;

  // Put running process to sleep
  proc->budget = proc->budget - (ticks - proc->cpu_ticks_in);
  if(proc->budget <= 0){
    proc->budget = MAX_BUDGET;
    if(proc->priority < MAXPRIO)
      proc->priority++;
  }
  changeState(&ptable.pLists.running, &ptable.pLists.runningTail, &ptable.pLists.sleep, &ptable.pLists.sleepTail, proc, RUNNING, SLEEPING, "sleep()");

  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}
#endif

//PAGEBREAK!
#ifndef CS333_P3P4
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
#else
static void
wakeup1(void *chan)
{
  struct proc *p;

  // Search sleep list for process to wakeup
  for(p = ptable.pLists.sleep; p != 0; p = p->next){
    if(p->chan == chan){
      changeState(&ptable.pLists.sleep, &ptable.pLists.sleepTail, &ptable.pLists.ready[p->priority], &ptable.pLists.readyTail[p->priority], p, SLEEPING, RUNNABLE, "wakeup1()");
    }
  }
}
#endif

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
#ifndef CS333_P3P4
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
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
#else
int
kill(int pid)
{
  int rc;

  acquire(&ptable.lock);

  rc = killSearchAll(&ptable.pLists.embryo, pid);
  if(rc == -1)
    rc = killSearchReady(pid);
  if(rc == -1)
    rc = killSearchAll(&ptable.pLists.sleep, pid);
  if(rc == -1)
    rc = killSearchAll(&ptable.pLists.running, pid);
  if(rc == -1)
    rc = killSearchAll(&ptable.pLists.zombie, pid);

  release(&ptable.lock);
  return rc;
}

// kill()s searchAll helper
static int
killSearchAll(struct proc** head, int pid)
{
  struct proc* p;
  struct proc* pK;

  for(p = *head; p != 0; p = p->next){
    if(p->pid == pid){
      p->killed = 1;
      for(pK = ptable.pLists.sleep; pK != 0; pK = pK->next){
        if(p == pK){
          changeState(&ptable.pLists.sleep, &ptable.pLists.sleepTail, &ptable.pLists.ready[0], &ptable.pLists.readyTail[0], p, SLEEPING, RUNNABLE, "killSearchAll()");
        }
        return 0;
      }
    }
  }
  return -1;
}

static int
killSearchReady(int pid)
{
  struct proc* p;
  struct proc* pK;
  int i;

  for(i=0; i < MAXPRIO+1; i++){
    for(p = ptable.pLists.ready[i]; p != 0; p = p->next){
      if(p->pid == pid){
        p->killed = 1;
        for(pK = ptable.pLists.sleep; pK != 0; pK = pK->next){
          if(p == pK){
            changeState(&ptable.pLists.sleep, &ptable.pLists.sleepTail, &ptable.pLists.ready[0], &ptable.pLists.readyTail[0], p, SLEEPING, RUNNABLE, "killSearchAll()");
          }
          return 0;
        }
      }
    }
  }
  return -1;
}
#endif

static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
};

#if defined(CS333_P3P4)
// Procdump helper for project 4
static void
procdumpP4(struct proc *p, char *state)
{
  uint elapsed_time = ticks - p->start_ticks;
  uint elapsed_secs = elapsed_time / 1000;
  uint elapsed_mils = elapsed_time % 1000;
  uint cpu_secs     = p->cpu_ticks_total / 1000;
  uint cpu_mils     = p->cpu_ticks_total % 1000;
  uint ppid         = 0;

  if(p->parent)
    ppid = p->parent->pid;
  else
    ppid = p->pid;

  if(strlen(p->name) < 8)
    cprintf("%d\t%s\t\t%d\t%d\t%d\t%d\t", p->pid, p->name, p->uid, p->gid, ppid, p->priority);
  else
    cprintf("%d\t%s\t%d\t%d\t%d\t%d\t", p->pid, p->name, p->uid, p->gid, ppid, p->priority);

  if((elapsed_mils < 1000) && (elapsed_mils > 99))
    cprintf("%d.%d\t  ", elapsed_secs, elapsed_mils);
  else if((elapsed_mils < 100) && (elapsed_mils > 9))
    cprintf("%d.0%d\t  ", elapsed_secs, elapsed_mils);
  else
    cprintf("%d.00%d\t  ", elapsed_secs, elapsed_mils);

  if((cpu_mils < 1000) && (cpu_mils > 99))
    cprintf("%d.%d\t\t", cpu_secs, cpu_mils);
  else if((cpu_mils < 100) && (cpu_mils > 9))
    cprintf("%d.0%d\t\t", cpu_secs, cpu_mils);
  else
    cprintf("%d.00%d\t\t", cpu_secs, cpu_mils);

  cprintf("%s\t%d\t", state, p->sz);
}

#elif defined(CS333_P2)
// Procdump helper for project 2
static void
procdumpP2(struct proc *p, char *state)
{
  uint elapsed_time = ticks - p->start_ticks;
  uint elapsed_secs = elapsed_time / 1000;
  uint elapsed_mils = elapsed_time % 1000;
  uint cpu_secs     = p->cpu_ticks_total / 1000;
  uint cpu_mils     = p->cpu_ticks_total % 1000;
  uint ppid         = 0;

  if(p->parent)
    ppid = p->parent->pid;
  else
    ppid = p->pid;

  if(strlen(p->name) < 8)
    cprintf("%d\t%s\t\t%d\t%d\t%d\t", p->pid, p->name, p->uid, p->gid, ppid);
  else
    cprintf("%d\t%s\t%d\t%d\t%d\t", p->pid, p->name, p->uid, p->gid, ppid);

  if((elapsed_mils < 1000) && (elapsed_mils > 99))
    cprintf("%d.%d\t  ", elapsed_secs, elapsed_mils);
  else if((elapsed_mils < 100) && (elapsed_mils > 9))
    cprintf("%d.0%d\t  ", elapsed_secs, elapsed_mils);
  else
    cprintf("%d.00%d\t  ", elapsed_secs, elapsed_mils);

  if((cpu_mils < 1000) && (cpu_mils > 99))
    cprintf("%d.%d\t\t", cpu_secs, cpu_mils);
  else if((cpu_mils < 100) && (cpu_mils > 9))
    cprintf("%d.0%d\t\t", cpu_secs, cpu_mils);
  else
    cprintf("%d.00%d\t\t", cpu_secs, cpu_mils);

  cprintf("%s\t%d\t", state, p->sz);
}

#elif defined(CS333_P1)
// Procdump helper for project 1
static void
procdumpP1(struct proc *p, char *state)
{
  uint time = ticks - p->start_ticks;
  uint secs = time / 1000;
  uint mils = time % 1000;

  cprintf("%d\t%s\t%s\t", p->pid, state, p->name);

  if((mils < 1000) && (mils > 99))
    cprintf("%d.%d\t", secs, mils);
  else if((mils < 100) && (mils > 9))
    cprintf("%d.0%d\t", secs, mils);
  else
    cprintf("%d.00%d\t", secs, mils);
}
#endif

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  #if defined(CS333_P3P4)
  #define HEADER "\nPID\tName\t\tUID\tGID\tPPID\tPrio\tElapsed\t  CPU\t\tState\tSize\t PCs\n"
  #elif defined(CS333_P2)
  #define HEADER "\nPID\tName\t\tUID\tGID\tPPID\tElapsed\t  CPU\t\tState\tSize\t PCs\n"
  #elif defined(CS333_P1)
  #define HEADER "\nPID\tState\tName\tElapsed\t PCs\n"
  #else
  #define HEADER ""
  #endif
  acquire(&ptable.lock);

  cprintf(HEADER);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

    #if defined(CS333_P3P4)
    procdumpP4(p, state);
    #elif defined(CS333_P2)
    procdumpP2(p, state);
    #elif defined(CS333_P1)
    procdumpP1(p, state);
    #else
    cprintf("%d %s %s", p->pid, state, p->name);
    #endif

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }

  release(&ptable.lock);
}

#ifdef CS333_P3P4
void
cready(void)
{
  acquire(&ptable.lock);
  struct proc *p;
  int i;

  cprintf("\nReady List Processes:\n");
  for(i=0; i < MAXPRIO+1; i++){
    if(i < 10)
      cprintf("%d:   ", i);
    else
      cprintf("%d:  ", i);
    for(p = ptable.pLists.ready[i]; p != 0; p = p->next){
      if(p->next)
        cprintf("(%d, %d) -> ", p->pid, p->budget);
      else
        cprintf("(%d, %d)\n\n", p->pid, p->budget);
    }
    if(!ptable.pLists.ready[i])
      cprintf("EMPTY\n\n");
  }
  release(&ptable.lock);
}

void
cfree(void)
{
  acquire(&ptable.lock);
  struct proc *p;
  int i = 0;

  for(p = ptable.pLists.free; p != 0; p = p->next){
    i++;
  }
  cprintf("\nFree List Size: %d Processes\n", i);
  release(&ptable.lock);
}

void
csleep(void)
{
  acquire(&ptable.lock);
  struct proc *p;

  cprintf("\nSleep List Processes:\n");
  for(p = ptable.pLists.sleep; p != 0; p = p->next){
    if(p->next)
      cprintf("%d -> ", p->pid);
    else
      cprintf("%d\n", p->pid);
  }
  if(!ptable.pLists.sleep)
    cprintf("EMPTY\n");
  release(&ptable.lock);
}

void
czombie(void)
{
  acquire(&ptable.lock);
  struct proc *p;
  uint ppID = 0;

  cprintf("\nZombie List Processes:\n");
  for(p = ptable.pLists.zombie; p != 0; p = p->next){
    if(p->parent)
      ppID = p->parent->pid;
    else
      ppID = p->pid;
    if(p->next)
      cprintf("(%d, %d) -> ", p->pid, ppID);
    else
      cprintf("(%d, %d)\n", p->pid, ppID);
  }
  if(!ptable.pLists.zombie)
    cprintf("EMPTY\n");
  release(&ptable.lock);
}

static void
assertState(struct proc* p, enum procstate state)
{
  char* pstate;
  if(p->state != state) {
    pstate = states[p->state];
    panic(pstate);
  }
}

static void
assertPrio(struct proc* p, uint prio)
{
  if(p->priority != prio)
    panic("Process removed from priority queue has incorrect priority. assertPrio()");
}

static void
changeState(struct proc** headRemove, struct proc** tailRemove, struct proc** headAdd, struct proc** tailAdd, struct proc* p, enum procstate stateRemove, enum procstate stateAdd, char* err)
{
  int rc;

  // Remove the process from source list, and assert it is valid for that list
  rc = stateListRemove(headRemove, tailRemove, p);
  if(rc == -1){
    panic(err);
  }
  assertState(p, stateRemove);

  // Change the processes state
  p->state = stateAdd;

  // Add to the destination list
  rc = stateListAdd(headAdd, tailAdd, p);
  if(rc == -1){
    panic(err);
  }
}

static int
stateListAdd(struct proc** head, struct proc** tail, struct proc* p)
{
  if (*head == 0) {
    *head = p;
    *tail = p;
    p->next = 0;
  } else {
    (*tail)->next = p;
    *tail = (*tail)->next;
    (*tail)->next = 0;
  }

  return 0;
}

static int
stateListRemove(struct proc** head, struct proc** tail, struct proc* p)
{
  if (*head == 0 || *tail == 0 || p == 0) {
    return -1;
  }

  struct proc* current = *head;
  struct proc* previous = 0;

  if (current == p) {
    *head = (*head)->next;
    return 0;
  }

  while(current) {
    if (current == p) {
      break;
    }

    previous = current;
    current = current->next;
  }

  // Process not found, hit eject.
  if (current == 0) {
    return -1;
  }

  // Process found. Set the appropriate next pointer.
  if (current == *tail) {
    *tail = previous;
    (*tail)->next = 0;
  } else {
    previous->next = current->next;
  }

  // Make sure p->next doesn't point into the list.
  p->next = 0;

  return 0;
}

static void
initProcessLists(void) {
  int i;
  for(i=0; i < MAXPRIO+1; i++){
    ptable.pLists.ready[i] = 0;
    ptable.pLists.readyTail[i] = 0;
  }
  ptable.pLists.free = 0;
  ptable.pLists.freeTail = 0;
  ptable.pLists.sleep = 0;
  ptable.pLists.sleepTail = 0;
  ptable.pLists.zombie = 0;
  ptable.pLists.zombieTail = 0;
  ptable.pLists.running = 0;
  ptable.pLists.runningTail = 0;
  ptable.pLists.embryo = 0;
  ptable.pLists.embryoTail = 0;
}

static void
initFreeList(void) {
  if (!holding(&ptable.lock)) {
    panic("acquire the ptable lock before calling initFreeList\n");
  }

  struct proc* p;

  for (p = ptable.proc; p < ptable.proc + NPROC; ++p) {
    p->state = UNUSED;
    stateListAdd(&ptable.pLists.free, &ptable.pLists.freeTail, p);
  }
}
#endif
