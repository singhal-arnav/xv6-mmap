#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int passed_tests = 0;

#define TEST(func) (func() ? (passed_tests++, printf(1, "TEST PASSED: %s\n", #func)) : printf(1, "TEST FAILED: %s\n", #func))

int create_test_file(const char *filename, const char *content, int size){
  int fd = open(filename, O_CREATE | O_RDWR);
  if(fd < 0){
    printf(2, "create_test_file: failed to create %s\n", filename);
    return -1;
  }

  if(content){
    int len = strlen(content);
    if(write(fd, content, len) != len){
      printf(2, "create_test_file: write failed\n");
      close(fd);
      return -1;
    }
  }

  if(size > 0){
    char buf[512];
    memset(buf, 'A', sizeof(buf));
    int written = content ? strlen(content) : 0;
    while(written < size){
      int to_write = (size - written) > sizeof(buf) ? sizeof(buf) : (size - written);
      if(write(fd, buf, to_write) != to_write){
        printf(2, "create_test_file: extend write failed\n");
        close(fd);
        return -1;
      }
      written += to_write;
    }
  }
  close(fd);
  return 0;
}


int test_read_only_mapping(){
  const char *filename = "test_readonly.txt";
  const char *content = "Hello, mmap world!";
  if(create_test_file(filename, content, 0) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_read_only_mapping: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_read_only_mapping: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  int success = 1;
  for (int i = 0; content[i]; i++){
    if(((char *)mapped)[i] != content[i]){
      printf(2, "test_read_only_mapping: content mismatch at %d\n", i);
      success = 0;
      break;
    }
  }

  munmap(mapped, 4096);
  close(fd);
  unlink(filename);
  return success;
}

int test_read_write_mapping(){
  const char *filename = "test_readwrite.txt";
  const char *content = "Original content here";
  if(create_test_file(filename, content, 0) < 0)
    return 0;

  int fd = open(filename, O_RDWR);
  if(fd < 0){
    printf(2, "test_read_write_mapping: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_read_write_mapping: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  strcpy(mapped, "Modified!");
  int success = (strcmp(mapped, "Modified!") == 0);
  if(!success){
    printf(2, "test_read_write_mapping: modification not visible\n");
  }

  munmap(mapped, 4096);
  close(fd);
  unlink(filename);
  return success;
}

int main(void){
  TEST(test_read_only_mapping);
  TEST(test_read_write_mapping);
  printf(1, "\n%d test cases passed\n", passed_tests);
  exit();
}