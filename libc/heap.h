#include <stddef.h>
#include <stdint.h>

typedef struct Halloc Halloc;

struct Halloc {
    void * start;
    void * end;
    size_t reserve_size;
    int (*grow_cb)(Halloc * cntx, unsigned int extra);
    void (*shrink_cb)(Halloc *cntx, uintptr_t addr);
    void (*dump_cb)(Halloc * cntx);
    int (*printf)(const char *, ...);
    void (*abort)(void);
};

void halloc_init(Halloc * cntx, void * start, unsigned int size);

void halloc_dump(const Halloc * cntx);
void halloc_dump2(const Halloc * cntx, void * highlight, const char * name);

void * halloc(Halloc * cntx, unsigned int size, int page_align, const char * tag);

void hfree(Halloc * cntx, void * ptr);

void * hrealloc(Halloc * cntx, void * buf, unsigned int size);

void halloc_integrity_check(const Halloc * cntx);
