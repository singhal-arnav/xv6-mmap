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

static struct mmap_node*
create_mmap_node(int start, int end, int prot, int flags, int offset, struct file *f, struct mmap_node *next)
{
  struct mmap_node *node = slab_alloc_node();
  node->start = start;
  node->end = end;
  node->prot = prot;
  node->flags = flags;
  node->offset = offset;
  node->f = f;
  node->next = next;
  filedup(f);
  return node;
}

int
sys_mmap(void)
{
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

  if(fd < 0 || fd >= NOFILE)
    return -1;

  f = myproc()->ofile[fd];
  if(!f)
    return -1;

  if((int)addr >= KERNBASE)
    return -1;

  length = PGROUNDUP(length);
  if(length <= 0)
    return -1;

  struct mmap_node *new_node, *curr, *prev;

  if(!myproc()->mmap_region){
    if(length > KERNBASE - myproc()->sz)
      return -1;
    new_node = create_mmap_node(KERNBASE - length, KERNBASE - 1, prot, flags, offset, f, 0);
    myproc()->mmap_region = new_node;
    return new_node->start;
  }

  curr = myproc()->mmap_region;
  if(length <= KERNBASE - curr->end - 1){
    new_node = create_mmap_node(KERNBASE - length, KERNBASE - 1, prot, flags, offset, f, curr);
    myproc()->mmap_region = new_node;
    return new_node->start;
  }

  prev = curr;
  curr = curr->next;
  while(curr){
    if(length <= prev->start - curr->end - 1){
      new_node = create_mmap_node(prev->start - length, prev->start - 1, prot, flags, offset, f, curr);
      prev->next = new_node;
      return new_node->start;
    }
    prev = curr;
    curr = curr->next;
  }

  if(length <= prev->start - myproc()->sz){
    new_node = create_mmap_node(prev->start - length, prev->start - 1, prot, flags, offset, f, 0);
    prev->next = new_node;
    return new_node->start;
  }
  return -1;
}

int
sys_munmap(void)
{
  int addr;
  int length;
  if(argint(0, &addr) < 0)
    return -1;
  if(argint(1, &length) < 0)
    return -1;

  struct mmap_node *curr, *prev;
  curr = myproc()->mmap_region;
  prev = 0;

  while(curr){
    if(curr->start == addr){
      if(!prev)
        myproc()->mmap_region = curr->next;
      else
        prev->next = curr->next;
      fileclose(curr->f);
      slab_free_node(curr);
      return 0;
    }
    prev = curr;
    curr = curr->next;
  }
  return -1;
}
