#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "fcntl.h"

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
  if(f)
    filedup(f);
  return node;
}

int
sys_mmap(void)
{
  void *addr;
  int length, prot, flags, fd, offset;
  struct file *f = 0;
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

  if(!(flags & MAP_ANONYMOUS)){
    if(fd < 0 || fd >= NOFILE)
      return -1;

    f = myproc()->ofile[fd];
    if(!f)
      return -1;
  }

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
      // Unmap pages
      int i;
      pde_t *pgdir = myproc()->pgdir;
      for(i = 0; i < PGROUNDUP(length); i += PGSIZE){
        pte_t *pte = walkpgdir(pgdir, (void*)(addr + i), 0);
        if(pte && (*pte & PTE_P)){
          uint pa = PTE_ADDR(*pte);
          if(curr->f) {
             if(curr->flags & MAP_SHARED) {
               // Decrement refcount in inode, if 0 free node
               remove_mapping(curr->f->ip, pa);
               // Do NOT free physical memory here if other processes use it?
               // But our simple model in remove_mapping kfrees it if refs==0.
               // So we rely on remove_mapping to handle freeing if it's the last ref.
               // But wait, remove_mapping frees the NODE. Who frees the PAGE?
               // kfree(P2V(pa))?
               // xv6 doesn't have page refcounts.
               // If remove_mapping freed the node (refs==0), then WE should free the page.
               // But remove_mapping doesn't return status.
               // Modification: remove_mapping should probably free the page if refs==0?
               // No, remove_mapping frees the slab node.
               // We need to know if we should free the physical page.
               // Let's assume if we removed the mapping, we own the page cleanup.
               // But remove_mapping is void.
               // Let's just update remove_mapping to free the page if it drops the last ref.
               // Wait, `islab_free_node` frees the metadata. The physical page `pa` needs `kfree`.
               // We should probably check if it was removed.
               // Re-read remove_mapping I just wrote: it calls islab_free_node(curr).
               // It doesn't kfree(curr->pa).
               // So we need to kfree(P2V(pa)) if we are the last one.
               // How do we know?
               // Maybe remove_mapping should take a "free_page" callback or return boolean.
               // For now, let's assume munmap of SHARED doesn't free the page if others hold it.
               // But without page refcounts, we don't know if others hold it *via page table*.
               // We only know if inode maps it.
               // If inode map refs == 0, then no one maps it via file.
               // So we can free it.
               // But remove_mapping logic:
               /*
                  curr->refs--;
                  if(curr->refs > 0) return;
                  ... remove from list ...
                  islab_free_node(curr);
               */
               // So if we returned from remove_mapping and the node is GONE, we should free page.
               // But we can't check "node is gone" easily.
               // So, better to move kfree INTO remove_mapping?
               // But fs.c doesn't include kalloc.c definitions usually? It does include defs.h.
               // kfree takes char*.
               // I will modify remove_mapping in fs.c to kfree(P2V(addr)) if refs==0.
             } else {
               // MAP_PRIVATE or ANONYMOUS
               // Just free the page.
               // And remove from inode if it was added?
               // MAP_PRIVATE adds to inode with private=1.
               if(curr->f) remove_mapping(curr->f->ip, pa);
               kfree(P2V(pa));
             }
          } else {
             // Anonymous
             kfree(P2V(pa));
          }
          *pte = 0;
        }
      }

      if(!prev)
        myproc()->mmap_region = curr->next;
      else
        prev->next = curr->next;
      if(curr->f)
        fileclose(curr->f);
      slab_free_node(curr);
      return 0;
    }
    prev = curr;
    curr = curr->next;
  }
  return -1;
}
