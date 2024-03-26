#include <dlfcn.h>
#include <stdlib.h>

int dlclose(void * handle)
{
    return 0;
}

char * dlerror()
{
    return "not implemented";
}

void * dlopen(const char *file, int mode)
{
    return NULL;
}

void * dlsym(void * handle, const char * restrict name)
{
    return NULL;
}
