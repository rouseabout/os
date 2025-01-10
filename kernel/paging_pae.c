#define PAE 1
#if defined(ARCH_i686)
#define BITS_PER_TABLE 9
#define PAGE_LEVELS 3
#else
#error unsupported architecture
#endif

#include "paging_pae.h"
#include "paging_template.c"
