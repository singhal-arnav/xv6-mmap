#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

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

  int npages = (length + PGSIZE - 1) / PGSIZE;
  int flag = 0, start, found = 0, va;

  if((int)addr >= KERNBASE)
    return 0;

  va = PGROUNDDOWN((int)addr);
  start = va;

  for(; va < KERNBASE; va += PGSIZE) {
    uint *pde, *pgtab, *pte;

    pde = &(myproc()->pgdir[PDX((char *)va)]);
    if(*pde & PTE_P){
      pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
    }
    else {
      if((pgtab = (pte_t*)kalloc()) == 0)
        return 0;
      memset(pgtab, 0, PGSIZE);
      *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
    }
    pte = &pgtab[PTX((char *)va)];

    int is_free;

    if(pte == 0 || (*pte & PTE_P) == 0)
      is_free = 1;
    else
      is_free = 0;

    if(is_free) {
      if(found == 0)
        start = va;
      found++;
      if(found >= npages) {
        flag = 1;
        break;
      }
    }
    else
      found = 0;

    cprintf("%d\n", start);
  }
  if(!flag || fileread(f, (char *)start, length) < 0)
    return -1;

  return start;
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
