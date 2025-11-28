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
  const char *content = "Aryan Jotshi";
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
  const char *content = "Aryan Jotshi";
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

  strcpy(mapped, "Aryan");
  int success = (strcmp(mapped, "Aryan") == 0);
  if(!success){
    printf(2, "test_read_write_mapping: modification not visible\n");
  }

  munmap(mapped, 4096);
  close(fd);
  unlink(filename);
  return success;
}

int test_page_aligned_mapping(){
  const char *filename = "test_aligned.txt";
  if(create_test_file(filename, "A", 4096) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_page_aligned_mapping: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_page_aligned_mapping: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  int success = (((char *)mapped)[0] == 'A');
  munmap(mapped, 4096);
  close(fd);
  unlink(filename);
  return success;
}

int test_unaligned_mapping(){
  const char *filename = "test_unaligned.txt";
  const char *content = "Aryan Jotshi";
  if(create_test_file(filename, content, 0) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_unaligned_mapping: open failed\n");
    unlink(filename);
    return 0;
  }

  int map_size = 100;
  void *mapped = mmap(0, map_size, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_unaligned_mapping: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  int success = (strcmp(mapped, content) == 0);
  munmap(mapped, map_size);
  close(fd);
  unlink(filename);
  return success;
}

int test_null_mapping(){
  const char *filename = "test_null.txt";
  const char *content = "Aryan Jotshi";
  if(create_test_file(filename, content, 0) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_null_mapping: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_null_mapping: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  int success = (mapped != 0 && strcmp(mapped, content) == 0);
  munmap(mapped, 4096);
  close(fd);
  unlink(filename);
  return success;
}

int test_multiple_mappings(){
  const char *filename = "test_multiple.txt";
  const char *content = "Aryan Jotshi";
  if(create_test_file(filename, content, 0) < 0)
    return 0;

  int fd = open(filename, O_RDWR);
  if(fd < 0){
    printf(2, "test_multiple_mappings: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped1 = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  void *mapped2 = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if(mapped1 == (void *)-1 || mapped2 == (void *)-1){
    printf(2, "test_multiple_mappings: mmap failed\n");
    if(mapped1 != (void *)-1) munmap(mapped1, 4096);
    if(mapped2 != (void *)-1) munmap(mapped2, 4096);
    close(fd);
    unlink(filename);
    return 0;
  }

  int success = (mapped1 != mapped2);
  strcpy(mapped1, "Changed");
  success = success && (((char *)mapped1)[0] == 'C');
  success = success && (((char *)mapped2)[0] != 0);
  munmap(mapped1, 4096);
  munmap(mapped2, 4096);
  close(fd);
  unlink(filename);
  return success;
}

int test_multi_page_mapping(){
  const char *filename = "test_multipage.txt";
  int file_size = 4096 * 3;
  if(create_test_file(filename, "Aryan", file_size) < 0)
    return 0;

  int fd = open(filename, O_RDWR);
  if(fd < 0){
    printf(2, "test_multi_page_mapping: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_multi_page_mapping: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }
  munmap(mapped, file_size);
  close(fd);
  unlink(filename);
  return 1;
}


int test_write_to_readonly(){
  const char *filename = "test_ro_write.txt";
  if(create_test_file(filename, "Aryan", 0) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_write_to_readonly: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_write_to_readonly: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  int success = (((char *)mapped)[0] == 'A');
  munmap(mapped, 4096);
  close(fd);
  unlink(filename);
  return success;
}

int test_munmap_invalidates(){
  const char *filename = "test_munmap.txt";
  if(create_test_file(filename, "Aryan", 0) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_munmap_invalidates: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_munmap_invalidates: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  int success = (((char *)mapped)[0] == 'A');
  if(munmap(mapped, 4096) < 0){
    printf(2, "test_munmap_invalidates: munmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }
  close(fd);
  unlink(filename);
  return success;
}

int test_invalid_fd(){
  void *mapped = mmap(0, 4096, PROT_READ, MAP_SHARED, -1, 0);
  int success = (mapped == (void *)-1);

  if(!success){
    printf(2, "test_invalid_fd: mmap should have failed\n");
    munmap(mapped, 4096);
  }
  return success;
}

int test_zero_length(){
  const char *filename = "test_zero.txt";
  if(create_test_file(filename, "Aryan", 0) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_zero_length: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, 0, PROT_READ, MAP_SHARED, fd, 0);
  int success = (mapped == (void *)-1);
  if(!success){
    printf(2, "test_zero_length: mmap should have failed\n");
    munmap(mapped, 0);
  }
  close(fd);
  unlink(filename);
  return success;
}

int test_close_fd_after_mmap(){
  const char *filename = "test_closefd.txt";
  const char *content = "Aryan Jotshi";
  if(create_test_file(filename, content, 0) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_close_fd_after_mmap: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_close_fd_after_mmap: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  close(fd);
  int success = (strcmp(mapped, content) == 0);
  munmap(mapped, 4096);
  unlink(filename);
  return success;
}

int test_map_beyond_file(){
  const char *filename = "test_beyond.txt";
  const char *content = "Aryan Jotshi";
  if(create_test_file(filename, content, 0) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_map_beyond_file: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_map_beyond_file: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  int success = (strcmp(mapped, content) == 0);
  munmap(mapped, 4096);
  close(fd);
  unlink(filename);
  return success;
}

int test_map_fixed_fails(){
  const char *filename = "test_fixed.txt";
  if(create_test_file(filename, "Aryan", 0) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_map_fixed_fails: open failed\n");
    unlink(filename);
    return 0;
  }

  void *hint = (void *)0x80000000;
  void *mapped = mmap(hint, 4096, PROT_READ, MAP_FIXED, fd, 0);
  int success = (mapped == (void *)-1);
  if(!success){
    printf(2, "test_map_fixed_fails: MAP_FIXED should have failed\n");
    munmap(mapped, 4096);
  }
  close(fd);
  unlink(filename);
  return success;
}

int test_partial_munmap(){
  const char *filename = "test_partial.txt";
  int size = 4096 * 2;
  if(create_test_file(filename, "Aryan", size) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf(2, "test_partial_munmap: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped == (void *)-1){
    printf(2, "test_partial_munmap: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }

  if(munmap(mapped, 4096) < 0){
    printf(2, "test_partial_munmap: partial munmap failed\n");
    munmap(mapped, size);
    close(fd);
    unlink(filename);
    return 0;
  }
  munmap((char *)mapped + 4096, 4096);
  close(fd);
  unlink(filename);
  return 1;
}

int test_many_mappings() {
  const char *filename = "test_many.txt";
  if (create_test_file(filename, "Many", 4096) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    printf(2, "test_many_mappings: open failed\n");
    unlink(filename);
    return 0;
  }

  #define NUM_MAPS 20
  void *mappings[NUM_MAPS];
  int success = 1;

  for (int i = 0; i < NUM_MAPS; i++) {
    mappings[i] = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0);
    if (mappings[i] == (void *)-1) {
      printf(2, "test_many_mappings: mmap %d failed\n", i);
      success = 0;
      break;
    }
  }

  if (success) {
    for (int i = 0; i < NUM_MAPS; i++) {
      if (((char *)mappings[i])[0] != 'M') {
        printf(2, "test_many_mappings: read from mapping %d failed\n", i);
        success = 0;
        break;
      }
    }
  }

  for (int i = 0; i < NUM_MAPS; i++) {
    if (mappings[i] != (void *)-1) {
      munmap(mappings[i], 4096);
    }
  }

  close(fd);
  unlink(filename);
  return success;
}

int test_large_file_mapping() {
  const char *filename = "test_large.txt";
  int size = 4096 * 16;
  
  if (create_test_file(filename, "LARGE", size) < 0)
    return 0;
  
  int fd = open(filename, O_RDWR);
  if (fd < 0) {
    printf(2, "test_large_file_mapping: open failed\n");
    unlink(filename);
    return 0;
  }

  void *mapped = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapped == (void *)-1) {
    printf(2, "test_large_file_mapping: mmap failed\n");
    close(fd);
    unlink(filename);
    return 0;
  }
  int success = 1;
  if (((char *)mapped)[0] != 'L') {
    success = 0;
  }

  for (int i = 0; i < 16; i++) {
    char *page = (char *)mapped + (i * 4096);
    page[0] = 'A' + i;
    if (page[0] != 'A' + i) {
      printf(2, "test_large_file_mapping: page %d access failed\n", i);
      success = 0;
      break;
    }
  }

  ((char *)mapped)[size - 1] = 'Z';
  if (((char *)mapped)[size - 1] != 'Z') {
    success = 0;
  }
  munmap(mapped, size);
  close(fd);
  unlink(filename);
  return success;
}

int test_rapid_map_unmap() {
  const char *filename = "test_rapid.txt";
  if (create_test_file(filename, "Rapid", 4096) < 0)
    return 0;

  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    printf(2, "test_rapid_map_unmap: open failed\n");
    unlink(filename);
    return 0;
  }

  int success = 1;
  for (int i = 0; i < 50; i++) {
    void *mapped = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0);
    if (mapped == (void *)-1) {
      printf(2, "test_rapid_map_unmap: mmap iteration %d failed\n", i);
      success = 0;
      break;
    }

    if (((char *)mapped)[0] != 'R') {
      printf(2, "test_rapid_map_unmap: read iteration %d failed\n", i);
      success = 0;
      munmap(mapped, 4096);
      break;
    }
    if (munmap(mapped, 4096) < 0) {
      printf(2, "test_rapid_map_unmap: munmap iteration %d failed\n", i);
      success = 0;
      break;
    }
  }
  close(fd);
  unlink(filename);
  return success;
}

int main(void){
  TEST(test_read_only_mapping);
  TEST(test_read_write_mapping);
  TEST(test_page_aligned_mapping);
  TEST(test_unaligned_mapping);
  TEST(test_null_mapping);
  TEST(test_multiple_mappings);
  TEST(test_multi_page_mapping);
  TEST(test_write_to_readonly);
  TEST(test_munmap_invalidates);
  TEST(test_invalid_fd);
  TEST(test_zero_length);
  TEST(test_close_fd_after_mmap);
  TEST(test_map_beyond_file);
  TEST(test_map_fixed_fails);
  TEST(test_partial_munmap);
  TEST(test_many_mappings);
  TEST(test_large_file_mapping);
  TEST(test_rapid_map_unmap);

  printf(1, "\n%d/18 test cases passed\n", passed_tests);
  exit();
}