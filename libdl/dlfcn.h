#ifndef DLFCN_H
#define DLFCN_H

#ifdef __cplusplus
extern "C" {
#endif

#define RTLD_GLOBAL 1
#define RTLD_NOW 2
#define RTLD_LAZY 3

#define RTLD_DEFAULT 0

int dlclose(void *);
char * dlerror(void);
void * dlopen(const char *, int);
void * dlsym(void *, const char * restrict);

#ifdef __cplusplus
}
#endif

#endif /* DLFCN_H */
