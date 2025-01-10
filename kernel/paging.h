#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

typedef struct page_directory page_directory;
typedef struct page_entry page_entry;

void alloc_frame(page_entry * page, int flags);
void free_frame(page_entry *page);
page_entry * get_page_entry(uintptr_t address, int make, page_directory * dir, int * called_alloc, int use_reserve);
uint64_t page_get_phy_address(const page_entry *page);
void set_frame_identity(page_entry * page, uint64_t addr, int flags);

page_directory * alloc_new_page_directory(int use_reserve);
page_directory * clone_directory(const page_directory * src);
void clean_directory(page_directory * dir, int free_all);
void dump_directory(const page_directory * dir, const char * name, int hexdump);

void switch_page_directory(page_directory * dir);
void load_user_pages(const page_directory * src);
void clear_user_pages(void);

extern uintptr_t clone_vaddr;
extern page_entry * clone_pe;

#endif
