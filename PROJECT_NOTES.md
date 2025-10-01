## What to do in mmap?
1. Complete all syscall setup steps apart from defining the kernel functions themselves
2. Implement a page fault handler
3. Come up with some logic for mapping a file to virtual memory
4. Make sure that if multiple threads/processes access the same file using mmap, they should be accessing the same pages in memory

NOTE: Understand and implement all options. mmap does not call read or write

How does mmap work?
- func sign: `void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);`
	- `addr`
		- Suggested starting address of mapping.
		- Usually ignored unless MAP_FIXED is specified.
		- Entering NULL will lead the kernel to allocate whatever chunk it can find
	- `len`
		- Size (in bytes) of mapping, will be rounded up to page size.
	- `prot` (protection flags)
		- check file for FLAGS `/usr/include/bits/mman-linux.h`
		- Defines memory protection of mapped region:
			- PROT_NONE → No access
			- PROT_READ → Pages can be read
			- PROT_WRITE → Pages can be written
			- PROT_EXEC → Pages can be executed
		- (May be bitwise OR-ed together.)
	- `flags` (mapping type + options)
		- Two main mapping types:
			- MAP_SHARED → Updates to the mapping are visible to other processes mapping the same file; written back to file.
			- MAP_PRIVATE → Copy-on-write mapping (changes are private, not written back).
		- Other common flags:
			- MAP_FIXED → Place mapping exactly at addr. (Dangerous — replaces existing mappings).
			- MAP_ANONYMOUS (or MAP_ANON) → Mapping is not backed by any file, instead uses zero-filled memory.
			- MAP_POPULATE → Prefault pages (not demand paging).
			- MAP_GROWSDOWN → Mapping is for stack (grows downward).
		```bash
		#define MAP_GROWSDOWN		0x00100		/* Stack-like segment. */
		#define MAP_DENYWRITE		0x00800		/* ETXTBSY. */
		#define MAP_EXECUTABLE		0x01000		/* Mark it as an executable. */
		#define MAP_LOCKED			0x02000		/* Lock the mapping. */
		#define MAP_NORESERVE		0x04000		/* Don\'t check for reservations. */
		#define MAP_POPULATE		0x08000		/* Populate (prefault) pagetables. */
		#define MAP_NONBLOCK		0x10000		/* Do not block on IO. */
		#define MAP_STACK			0x20000		/* Allocation is for a stack. */
		#define MAP_HUGETLB			0x40000		/* Create huge page mapping. */
		#define MAP_SYNC			0x80000		/* Perform synchronous page faults for the mapping. */
		#define MAP_FIXED_NOREPLACE 0x100000	/* MAP_FIXED but do not unmap underlying mapping. */
		```
	- `fildes`
		- File descriptor (ignored if MAP_ANONYMOUS).
	- `off`
		- File offset (must be page-aligned).

- Start by checking the value of addr. NULL means file can be mapped anywhere, other value means file must be mapped at that virtual address. Look for a large enough chunk in the process's virtual address space. Theoretically, this could be done using a sliding window approach to scan the page tables. However, this is too slow and time-consuming. Linux maintains something known as a VMA (virtual memory areas) structure for each process. A similar structure will have to be implemented here.

- mmap only creates a mapping between the virtual memory region and the relevant bytes in the file. It does not call sys_read(), sys_write(), fileread() or filewrite(). Anything that must be read into memory will automatically be read when the page fault handler gets called upon first access.

- Once a suitable region is found, update the VMA structure. Mark page table entries as invalid so that the first access will trigger a page fault. If a write is performed, the page must be marked dirty.

- When a page is being read in or written back, you can use fileread() or filewrite().

- xv6 does not handle dirty pages automatically. Since all options are to be implemented, dirty page tracking and handling must be implemented manually.