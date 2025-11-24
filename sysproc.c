#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

struct file;

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
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
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
    if(myproc()->killed){
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

int
sys_mmap(void)
{
  cprintf("sys_mmap called!\n");
  void *addr;
  int length, prot, flags, fd, offset;
  struct file *f;
  if(argptr(0, (char **)&addr, sizeof(void *)) < 0)
    return -1;
  if(argint(1, &length) < 0)
    return -1;
  if(argint(2, &prot) < 0)
    return -1;
  if(argint(3, &flags) < 0)
    return -1;
  if(argint(4, &fd) < 0)
    return -1;
  if(argint(5, &offset) < 0)
    return -1;

  f = myproc()->ofile[fd];

  if((int)addr >= KERNBASE)
    return 0;

  struct mmap_node *n, *p;

  p = myproc()->mmap_region;
  while(p && p->next) {
    if((p->next->start - p->end - 1) >= length) {
      n = slab_alloc_node();
      n->start = p->end + 1;
      n->end = PGROUNDUP(n->start + length) - 1;

      n->prot = prot;
      n->flags = flags;
      n->offset = offset;
      n->f = f;
      filedup(f);

      n->next = p->next;
      p->next = n;
      return n->start;
    }
    p = p->next;
  }

  n = slab_alloc_node();
  if(!p)
    n->start = PGROUNDDOWN(KERNBASE - length);
  else
    n->start = PGROUNDDOWN(myproc()->mmap_region->start - length);

  if(n->start >= myproc()->sz) {
    n->end = PGROUNDUP(n->start + length) - 1;
    n->next = myproc()->mmap_region;
    myproc()->mmap_region = n;

    n->prot = prot;
    n->flags = flags;
    n->offset = offset;
    n->f = f;
    filedup(f);

    return n->start;
  }
  else {
    slab_free_node(n);
    return -1;
  }
}

int
sys_munmap(void)
{
  cprintf("sys_munmap called!\n");
  void *addr;
  int length;
  if(argptr(0, (char**)&addr, sizeof(void *)) < 0) return -1;
  if(argint(1, &length) < 0) return -1;
  return 0;
}
