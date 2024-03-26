#include <stddef.h>
#include "vfs.h"

void * ata_init(void);
int ata_getsize(void * priv_data);
extern const DeviceOperations ata_dio;
