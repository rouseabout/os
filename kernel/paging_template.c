
extern page_directory *kernel_directory;
extern uintptr_t clone_vaddr;
extern page_entry * clone_pe;


#include "utils.h"
#include <string.h>
#include <stdlib.h>

#include <stdint.h>

#define ENTRIES_PER_TABLE (1 << BITS_PER_TABLE)

#if PAE
#define uintptr_t uint64_t
#endif
struct page_entry {
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
#if defined(ARCH_i686)
#if PAE
    uintptr_t frame    : 52;
#else
    uintptr_t frame    : 20;
#endif
#elif defined(ARCH_x86_64)
    uintptr_t frame    : 52;
#endif
};
#if PAE
#undef uintptr_t
#endif

typedef struct {
    page_entry pages[ENTRIES_PER_TABLE];
} page_table;

struct page_directory {
    /* because kmalloc_ap only gives physical adddress of the first page, must place tables_physical[] array first */
    page_entry tables_physical[ENTRIES_PER_TABLE];  //physical address of tables
    page_directory * tables[ENTRIES_PER_TABLE];  //virtual address to page_directory or page_table struct
    uintptr_t physical_address;  //physical address; points to tables_physical
};

static void indent(int level)
{
    level = PAGE_LEVELS - level;
    while(level--) kprintf("  ");
}

static void dump_directory_r(const page_directory * dir, const page_directory * kd, const char * name, int hexdump, int level, uintptr_t base)
{
    indent(level); kprintf("DIRECTORY level %d '%s' virtual-addr=%p", level, name, dir);
    if (level >= 2)
        kprintf(", physical-addr=%p", dir->physical_address);
    kprintf("\n");
    for (unsigned int i = 0; i < ENTRIES_PER_TABLE; i++) {
        //KASSERT(dir->tables_physical[i].present ^ !dir->tables[i]);
        uintptr_t base2 = base | (uintptr_t)i << (12 + (level-1)*BITS_PER_TABLE);

        if (dir->tables_physical[i].present) {
            indent(level); kprintf("tables_physical[%d] = %p phy (<- %p virt) user:%d, rw:%d\n", i, dir->tables_physical[i], base2, dir->tables_physical[i].user, dir->tables_physical[i].rw);
        }
        if (level >= 2 && dir->tables[i]) {
            indent(level); kprintf("tables[%d] = %p (<- %p virt) %s\n", i, dir->tables[i], base2, kd && (dir->tables[i] == kd->tables[i]) ? "*" : "");
            dump_directory_r(dir->tables[i], kd ? kd->tables[i] : NULL, name, hexdump, level - 1, base2);
        }
    }
}

void RENAME(dump_directory)(const page_directory * dir, const char * name, int hexdump)
{
    dump_directory_r(dir, kernel_directory, name, hexdump, PAGE_LEVELS, 0);
    kprintf("\n");
}

void RENAME(load_user_pages)(const page_directory * src)
{
    for (unsigned int i = 0; i < ENTRIES_PER_TABLE; i++) {
        uintptr_t base2 = (uintptr_t)i << (12 + (PAGE_LEVELS - 1)*BITS_PER_TABLE);
        if (base2 >= KERNEL_START)
            break;
        kernel_directory->tables_physical[i] = src->tables_physical[i];
        kernel_directory->tables[i] = src->tables[i];
    }
    RENAME(switch_page_directory)(kernel_directory);
}

void RENAME(clear_user_pages)()
{
    for (unsigned int i = 0; i < ENTRIES_PER_TABLE; i++) {
        uintptr_t base2 = (uintptr_t)i << (12 + (PAGE_LEVELS - 1)*BITS_PER_TABLE);
        if (base2 >= KERNEL_START)
            break;
        kernel_directory->tables_physical[i].present = 0;
        kernel_directory->tables[i] = 0;
    }
}

void RENAME(alloc_frame)(page_entry * page, int flags)
{
    if (page->present)
        panic("alloc_frame: page already present\n");

    page->present = 1;
    page->rw      = !!(flags & MAP_WRITABLE);
    page->user    = !!(flags & MAP_USER);
    page->frame   = first_frame();
    set_frame(page->frame * 0x1000);
}

void RENAME(free_frame)(page_entry *page)
{
    if (!page->present)
        panic("free_frame: page not present");

    clear_frame(page->frame * 0x1000);
    page->present = 0;
    page->frame   = 0xBEEF;
}

void RENAME(set_frame_identity)(page_entry * page, uint64_t addr, int flags)
{
    if (page->present)
        panic("set_frame_identity: page already present\n");

    page->present = 1;
    page->rw      = !!(flags & MAP_WRITABLE);
    page->user    = !!(flags & MAP_USER);
    if (cpu_has_pat) {
        page->nocache = 0;
        page->writethrough = 0;
        page->pat = !!(flags & MAP_WRITETHROUGH);
    }
    page->frame   = addr >> 12;
}

page_directory * RENAME(alloc_new_page_directory)(int use_reserve)
{
    uintptr_t phy;
    page_directory * dir = (page_directory *)kmalloc_ap2(sizeof(page_directory), &phy, use_reserve, "pg-dir");
    memset(dir->tables_physical, 0, sizeof(dir->tables_physical));
    memset(dir->tables, 0, sizeof(dir->tables));
    dir->physical_address = phy;
    return dir;
}

static void * RENAME(alloc_new_page_table)(uintptr_t * phy, int use_reserve)
{
    page_table * pt = (page_table *)kmalloc_ap2(sizeof(page_table), phy, use_reserve, "pg-table");
    memset(pt, 0, sizeof(page_table));
    return pt;
}

page_entry * RENAME(get_page_entry)(uintptr_t address, int make, page_directory * dir, int * called_alloc, int use_reserve)
{
    address /= 0x1000;
    for (int level = PAGE_LEVELS - 1 ; level > 0; level--) {
        int idx = (address >> (level * BITS_PER_TABLE)) % ENTRIES_PER_TABLE;
        if (!dir->tables[idx]) {
            //kprintf(" -> (get_page_entry %p virt) level %d, idx %d missing\n", address * 0x1000, level + 1, idx);
            if (!make)
                return NULL;
            if (called_alloc)
                *called_alloc = 1;
            uintptr_t phy;
            if (level + 1 == 2) {
                dir->tables[idx] = RENAME(alloc_new_page_table)(&phy, use_reserve);
            } else {
                dir->tables[idx] = RENAME(alloc_new_page_directory)(use_reserve);
                phy = dir->tables[idx]->physical_address;
            }
            //kprintf(" -> new table %p virt (%p phy)\n", dir->tables[idx], phy);
            dir->tables_physical[idx].present = 1;
            if (!PAE || level < PAGE_LEVELS - 1) {
                dir->tables_physical[idx].rw = 1;
                dir->tables_physical[idx].user = 1; //FIXME:
            }
            dir->tables_physical[idx].frame = phy / 0x1000;
        }
        dir = dir->tables[idx];
    }

    /* level 1: only contains table_physical[] */
    int idx = address % ENTRIES_PER_TABLE;
    return &dir->tables_physical[idx];
}

uint64_t RENAME(page_get_phy_address)(const page_entry *page)
{
    return page->frame * 0x1000;
}


static page_table * RENAME(clone_table)(const page_table * src, uintptr_t * physical_address, uintptr_t base)
{
    page_table * table = (page_table *)kmalloc_ap(sizeof(page_table), physical_address, "pg-table-clone");
    //kprintf("page_table %p (%d bytes)\n", table, sizeof(page_table));
    if (!table)
        return NULL;
    memset(table, 0, sizeof(page_table));

    // calculate memory required to clone page table
    int npages = 0;
    for (unsigned int i = 0; i < ENTRIES_PER_TABLE; i++)
        if (src->pages[i].present)
            npages++;

    if (nframes - count_used_frames() < npages) {
        kfree(table);
        return NULL;
    }

    clone_pe->present = 1;
    clone_pe->user = 0;
    clone_pe->rw = 1;

    //copy each entry
    for (unsigned int i = 0; i < ENTRIES_PER_TABLE; i++) {

        if (!src->pages[i].present)
            continue;

        RENAME(alloc_frame)(&table->pages[i], 0);

#define _(x) if (src->pages[i].x) table->pages[i].x = 1;
_(present)
_(rw)
_(user)
_(accessed)
_(dirty)
#undef _

        /* copy page content */
        uintptr_t psrc = base | (i << 12);
#if 0
        const page_entry * pe_src = get_page_entry(psrc, 0, current_directory);
        KASSERT(pe_src && src->pages[i].frame == pe_src->frame);
#endif
        clone_pe->frame = table->pages[i].frame; /* temporarily map destination physical frame */
        RENAME(switch_page_directory)(kernel_directory); /* reload */
        memcpy((void *)clone_vaddr, (void *)(uintptr_t)psrc, 0x1000);
    }

    return table;
}

static int clean_directory_r(page_directory * dir, const page_directory * kd, int level, uintptr_t base, int free_all);

static page_directory * clone_directory_r(const page_directory * src, const page_directory * kd, uintptr_t * pphys, int level, uintptr_t base)
{
    page_directory * dir = (page_directory *)kmalloc_ap(sizeof(page_directory), pphys, "pg-dir-clone");
    if (!dir)
        return NULL;
    memset(dir, 0, sizeof(page_directory));
    dir->physical_address = *pphys;

    //copy each page
    for (unsigned int i = 0; i < ENTRIES_PER_TABLE; i++) {
        uintptr_t base2 = base | (uintptr_t)i << (12 + (level-1)*BITS_PER_TABLE);

        if (!src->tables[i])
            continue;
        if (level == 2 && kd && src->tables[i] == kd->tables[i] && base2 >= KERNEL_START) {
            dir->tables[i] = src->tables[i];
            dir->tables_physical[i] = src->tables_physical[i];
        } else {
            uintptr_t phys;
            dir->tables[i] = level > 2 ? clone_directory_r(src->tables[i], kd ? kd->tables[i] : NULL, &phys, level - 1, base2) : (page_directory *)RENAME(clone_table)((const page_table *)src->tables[i], &phys, base2) ;
            if (!dir->tables[i]) {
                clean_directory_r(dir, kd, level, base, 1);
                kfree(dir);
                return NULL;
            }
            dir->tables_physical[i].present = 1;
            if (!PAE || level != PAGE_LEVELS) {
                dir->tables_physical[i].rw = 1;
                dir->tables_physical[i].user = 1; //FIXME:
            }
            dir->tables_physical[i].frame = phys / 0x1000;
        }
    }

    return dir;
}

page_directory * RENAME(clone_directory)(const page_directory * src)
{
    uintptr_t phys;
    return clone_directory_r(src, (page_directory *)kernel_directory, &phys, PAGE_LEVELS, 0);
}

/* remove all non-kernel pages from the directory */

static void RENAME(clean_table)(page_table * table)
{
    for (unsigned int i = 0; i < ENTRIES_PER_TABLE; i++) {
        if (table->pages[i].present)
            RENAME(free_frame)(&table->pages[i]);
    }
}

static int clean_directory_r(page_directory * dir, const page_directory * kd, int level, uintptr_t base, int free_all)
{
    int empty = 1;
    for (unsigned int i = 0; i < ENTRIES_PER_TABLE; i++) {
        uintptr_t base2 = base | (uintptr_t)i << (12 + (level-1)*BITS_PER_TABLE);
        if (!dir->tables[i])
            continue;
        if (kd && dir->tables[i] == kd->tables[i] && base2 >= KERNEL_START) {
            empty = 0;
            continue;
        }
        int free_this;
        if (level > 2) {
            free_this = clean_directory_r(dir->tables[i], kd ? kd->tables[i] : NULL, level - 1, base2, free_all);
        } else {
            RENAME(clean_table)((page_table *)dir->tables[i]);
            free_this = 1;
        }
        if (free_this || free_all) {
            kfree(dir->tables[i]);
            dir->tables[i] = NULL;
            dir->tables_physical[i].present = 0;
        } else {
            empty = 0;
        }
    }
    return empty;
}

void RENAME(clean_directory)(page_directory * dir, int free_all)
{
    clean_directory_r(dir, kernel_directory, PAGE_LEVELS, 0, free_all);
    if (free_all)
        kfree(dir);
}

void RENAME(switch_page_directory)(page_directory * dir)
{
    asm volatile("mov %0, %%cr3" : : "r" (dir->physical_address));
}
