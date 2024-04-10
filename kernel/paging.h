#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#if defined(ARCH_i686)
#define BITS_PER_TABLE 10
#define PAGE_LEVELS 2
#elif defined(ARCH_x86_64)
#define BITS_PER_TABLE 9
#define PAGE_LEVELS 4
#else
#error unsupported architecture
#endif

#define ENTRIES_PER_TABLE (1 << BITS_PER_TABLE)

typedef struct {
    uintptr_t present  :  1;
    uintptr_t rw       :  1;
    uintptr_t user     :  1;
    uintptr_t writethrough :  1;
    uintptr_t nocache  :  1;
    uintptr_t accessed :  1;
    uintptr_t dirty    :  1;
    uintptr_t pat      :  1;
    uintptr_t global   :  1;
    uintptr_t unused   :  3;
    uintptr_t frame    : 20;
} page_entry;

typedef struct {
    page_entry pages[ENTRIES_PER_TABLE];
} page_table;

typedef struct page_directory page_directory;
struct page_directory {
    /* because kmalloc_ap only gives physical adddress of the first page, must place tables_physical[] array first */
    page_entry tables_physical[ENTRIES_PER_TABLE];  //physical address of tables
    page_directory * tables[ENTRIES_PER_TABLE];  //virtual address to page_directory or page_table struct
    uintptr_t physical_address;  //physical address; points to tables_physical
};

extern page_directory *kernel_directory;
extern page_directory *current_directory;

page_entry * get_page_entry(uintptr_t address, int make, page_directory * dir, int * called_alloc, int use_reserve);

#endif
