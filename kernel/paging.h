#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

typedef int word;

typedef struct {
    uint32_t present  :  1;
    uint32_t rw       :  1;
    uint32_t user     :  1;
    uint32_t writethrough :  1;
    uint32_t nocache  :  1;
    uint32_t accessed :  1;
    uint32_t dirty    :  1;
    uint32_t pat      :  1;
    uint32_t global   :  1;
    uint32_t unused   :  3;
    uint32_t frame    : 20;
} page_entry;

typedef struct {
    page_entry pages[1024];
} page_table;

typedef struct {
    /* because kmalloc_ap only gives physical adddress of the first page, must place tables_physical[] array first */
    uint32_t tables_physical[1024];  //physical address of tables
    page_table * tables[1024];  //virtual address of tables
    word physical_address;  //physical address; points to tables_physical
} page_directory;

extern page_directory *kernel_directory;
extern page_directory *current_directory;

page_entry * get_page_entry(uint32_t address, int make, page_directory * dir);

#endif
