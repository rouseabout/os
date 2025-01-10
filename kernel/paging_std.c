#if defined(ARCH_i686)
#define BITS_PER_TABLE 10
#define PAGE_LEVELS 2
#elif defined(ARCH_x86_64)
#define BITS_PER_TABLE 9
#define PAGE_LEVELS 4
#else
#error unsupported architecture
#endif

#include "paging_template.c"
