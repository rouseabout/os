#include <stddef.h>
#include "vfs.h"

void * ac97_init(void);
void ac97_poll(void * cntx);
extern const DeviceOperations ac97_dio;
