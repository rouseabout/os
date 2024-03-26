#include "vfs.h"

#define PROC_INODE_MIN 100000
#define PROC_INODE_MAX 199999

void proc_init(void);
void proc_register_file(const char * name, void (*cb)(FileDescriptor * fd));

extern const FileOperations proc_io;
