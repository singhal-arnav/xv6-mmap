#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main (void)
{
  int fd;
  char *addr = 0;

  fd = open ("README", O_RDONLY);
  if(fd < 0){
    printf(2, "test_mmap: failed to open file\n");
    exit();
  }

  int length = 4096*2 + 3;

  void *mapped = mmap(addr, length, PROT_READ | PROT_WRITE, 0, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_mmap: mmap failed\n");
    close(fd);
    exit();
  }

  printf(1, "test_mmap: mapping successful at %p\n", mapped);

  printf(1, "First Byte: %c\n", ((char *)mapped)[0]);
  // printf(1, "Data in mapped region: %s", mapped);

  strcpy(mapped, "Modified through mmap!\n");

  if(munmap(mapped, length) < 0){
    printf(2, "test_mmap: munmap failed\n");
    close(fd);
    exit();
  }

  printf(1, "test_mmap: unmapped successfully\n");

  close(fd);
  exit();
}