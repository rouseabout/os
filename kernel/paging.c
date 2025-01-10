#include "paging.h"

#include "paging_std.h"
#undef page_entry
#undef page_directory

#define GO(x) std_##x
#define ENTRY(x) ((std_page_entry *)(x))
#define DIRECTORY(x) ((std_page_directory *)(x))

uintptr_t clone_vaddr;
page_entry * clone_pe;

void alloc_frame(page_entry * page, int flags)
{
    GO(alloc_frame)(ENTRY(page), flags);
}

void free_frame(page_entry *page)
{
    GO(free_frame)(ENTRY(page));
}

page_entry * get_page_entry(uintptr_t address, int make, page_directory * dir, int * called_alloc, int use_reserve)
{
    return (page_entry *)GO(get_page_entry)(address, make, DIRECTORY(dir), called_alloc, use_reserve);
}

uint64_t page_get_phy_address(const page_entry *page)
{
    return GO(page_get_phy_address)(ENTRY(page));
}

void set_frame_identity(page_entry * page, uintptr_t addr, int flags)
{
    GO(set_frame_identity)(ENTRY(page), addr, flags);
}

page_directory * alloc_new_page_directory(int use_reserve)
{
    return (page_directory *)GO(alloc_new_page_directory)(use_reserve);
}

page_directory * clone_directory(const page_directory * src)
{
    return (page_directory *)GO(clone_directory)(DIRECTORY(src));
}

void clean_directory(page_directory * dir, int free_all)
{
    GO(clean_directory)(DIRECTORY(dir), free_all);
}

void dump_directory(const page_directory * dir, const char * name, int hexdump)
{
    GO(dump_directory)(DIRECTORY(dir), name, hexdump);
}

void switch_page_directory(page_directory * dir)
{
    GO(switch_page_directory)(DIRECTORY(dir));
}

void load_user_pages(const page_directory * src)
{
    GO(load_user_pages)(DIRECTORY(src));
}

void clear_user_pages(void)
{
    GO(clear_user_pages)();
}
