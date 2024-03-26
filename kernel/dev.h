#include "vfs.h"

#define DEV_INODE_MIN 200000
#define DEV_INODE_MAX 299999

void dev_init(void);
void dev_register_device(const char * name, const DeviceOperations * ops, int isatty, int (*getsize)(void * priv_data), void * priv_data);
void dev_register_symlink(const char * name, const char * symlink);

extern const FileOperations dev_io;
