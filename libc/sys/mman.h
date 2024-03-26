#ifndef SYS_MMAN_H
#define SYS_MMAN_H

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 4

#define MAP_FAILED 0

#define MS_SYNC 1

void * mmap(void *, size_t, int, int, int, off_t);
int mprotect(void *, size_t, int);
int msync(void *, size_t, int);
int munmap(void *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* SYS_MMAN_H */
