#include <stddef.h>
#include "vfs.h"

void * ext2_init(const char * device);
extern const FileOperations ext2_io;
