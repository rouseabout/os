#include <stdint.h>

typedef struct page_directory page_directory;
typedef struct page_entry page_entry;

void RENAME(alloc_frame)(page_entry * page, int flags);
void RENAME(free_frame)(page_entry *page);
page_entry * RENAME(get_page_entry)(uintptr_t address, int make, page_directory * dir, int * called_alloc, int use_reserve);
uint64_t RENAME(page_get_phy_address)(const page_entry *page);
void RENAME(set_frame_identity)(page_entry * page, uint64_t addr, int flags);

page_directory * RENAME(alloc_new_page_directory)(int use_reserve);
page_directory * RENAME(clone_directory)(const page_directory * src, const page_directory * kernel_directory);
void RENAME(clean_directory)(page_directory * dir, int free_all, const page_directory * kernel_directory);
void RENAME(dump_directory)(const page_directory * dir, const page_directory * kernel_directory, const char * name, int hexdump);

void RENAME(switch_page_directory)(const page_directory * dir);
void RENAME(load_user_pages)(page_directory * kernel_directory, const page_directory * src);
void RENAME(clear_user_pages)(page_directory * kernel_directory);
