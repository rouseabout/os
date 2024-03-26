#include <stddef.h>
#include "vfs.h"

void * mem_init(void * start, size_t size);
int mem_getsize(void * priv_data);
extern DeviceOperations mem_dio;
