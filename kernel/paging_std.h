#ifndef PAGING_STD_H
#define PAGING_STD_H

#define RENAME(x) std_##x
#define page_entry std_page_entry
#define page_directory std_page_directory
#include "paging_template.h"

#endif
