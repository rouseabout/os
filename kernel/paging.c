#include "utils.h"
#include "paging.h"

#include "paging_std.h"
#undef RENAME
#undef page_entry
#undef page_directory

#if defined(ARCH_i686)
#include "paging_pae.h"
#undef RENAME
#undef page_entry
#undef page_directory
#endif

#define GO(x) std_##x
#define ENTRY(x) ((std_page_entry *)(x))
#define DIRECTORY(x) ((std_page_directory *)(x))

uintptr_t clone_vaddr;
page_entry * clone_pe;

void alloc_frame(page_entry * page, int flags)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        pae_alloc_frame((pae_page_entry *)page, flags);
    else
#endif
        std_alloc_frame((std_page_entry *)page, flags);
}

void free_frame(page_entry *page)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        pae_free_frame((pae_page_entry *)page);
    else
#endif
        std_free_frame((std_page_entry *)page);
}

page_entry * get_page_entry(uintptr_t address, int make, page_directory * dir, int * called_alloc, int use_reserve)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        return (page_entry *)pae_get_page_entry(address, make, (pae_page_directory *)dir, called_alloc, use_reserve);
    else
#endif
        return (page_entry *)std_get_page_entry(address, make, (std_page_directory *)dir, called_alloc, use_reserve);
}

uint64_t page_get_phy_address(const page_entry *page)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        return pae_page_get_phy_address((pae_page_entry *)page);
    else
#endif
        return std_page_get_phy_address((std_page_entry *)page);
}

void set_frame_identity(page_entry * page, uint64_t addr, int flags)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        pae_set_frame_identity((pae_page_entry *)page, addr, flags);
    else
#endif
        std_set_frame_identity((std_page_entry *)page, addr, flags);
}

page_directory * alloc_new_page_directory(int use_reserve)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        return (page_directory *)pae_alloc_new_page_directory(use_reserve);
    else
#endif
        return (page_directory *)std_alloc_new_page_directory(use_reserve);
}

page_directory * clone_directory(const page_directory * src)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        return (page_directory *)pae_clone_directory((pae_page_directory *)src);
    else
#endif
        return (page_directory *)std_clone_directory((std_page_directory *)src);
}

void clean_directory(page_directory * dir, int free_all)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        pae_clean_directory((pae_page_directory *)dir, free_all);
    else
#endif
        std_clean_directory((std_page_directory *)dir, free_all);
}

void dump_directory(const page_directory * dir, const char * name, int hexdump)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        pae_dump_directory((pae_page_directory *)dir, name, hexdump);
    else
#endif
        std_dump_directory((std_page_directory *)dir, name, hexdump);
}

void switch_page_directory(page_directory * dir)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        pae_switch_page_directory((pae_page_directory *)dir);
    else
#endif
        std_switch_page_directory((std_page_directory *)dir);
}

void load_user_pages(const page_directory * src)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        pae_load_user_pages((pae_page_directory *)src);
    else
#endif
        std_load_user_pages((std_page_directory *)src);
}

void clear_user_pages(void)
{
#if defined(ARCH_i686)
    if (cpu_has_pae)
        pae_clear_user_pages();
    else
#endif
        std_clear_user_pages();
}
