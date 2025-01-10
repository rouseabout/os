#ifndef PAGING_PAE_H
#define PAGING_PAE_H

#define RENAME(x) pae_##x
#define page_entry pae_page_entry
#define page_directory pae_page_directory
#include "paging_template.h"

#endif
