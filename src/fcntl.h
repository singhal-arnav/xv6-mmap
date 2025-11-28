#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200

//mmap protection flags
#define PROT_NONE 	0x0
#define PROT_READ 	0x1
#define PROT_WRITE 	0x2
#define PROT_EXEC 	0x4
#define PROT_GROWSDOWN 	0x01000000
#define PROT_GROWSUP 	0x02000000

//mapping flags
#define MAP_FILE 		0
#define MAP_SHARED 		0x01
#define MAP_PRIVATE 		0x02
#define MAP_SHARED_VALIDATE 	0x03
#define MAP_DROPPABLE		0x08
#define MAP_TYPE 		0x0f
#define MAP_FIXED 		0x10
#define MAP_ANONYMOUS 		0x20
#define MAP_GROWSDOWN		0x00100
#define MAP_DENYWRITE		0x00800
#define MAP_EXECUTABLE		0x01000
#define MAP_LOCKED		0x02000
#define MAP_NORESERVE		0x04000
#define MAP_POPULATE		0x08000
#define MAP_NONBLOCK		0x10000
#define MAP_STACK		0x20000
#define MAP_HUGETLB		0x40000
#define MAP_SYNC		0x80000
#define MAP_FIXED_NOREPLACE 	0x100000
