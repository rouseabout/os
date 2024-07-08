#include "heap.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <bsd/string.h>
#include <errno.h>

#if TEST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define kprintf printf
static void panic(){exit(1);}
#define MEMORY_SIZE (1024*1024)
#endif

#define VALIDATE 0

#if VALIDATE
#define ASSERT(x) do { if (!(x)) { cntx->printf("%p: ASSERT FAILED: " __FILE__ ":%d\n", cntx, __LINE__); cntx->abort(); } } while(0)
#define HEAD_MAGIC 0x5a6978
#define TAIL_MAGIC 0x0f1e2d3c
#else
#define ASSERT(x) do { } while(0)
#endif

typedef struct {
#if VALIDATE
    uint32_t magic;
#endif
    size_t size;
    uint8_t align : 4;
    uint8_t used : 1;
#if VALIDATE
    char tag[16];
#endif
} Head;

typedef struct {
#if VALIDATE
    uint32_t magic;
#endif
    size_t size;
} Tail;

static Tail * head_tail(Head * h)
{
    return (Tail *)((uint8_t*)h + sizeof(Head) + h->size);
}

static Head * head_next(Head * h)
{
    return (Head *)((uint8_t*)h + sizeof(Head) + h->size + sizeof(Tail));
}

static void * head_data(Head * h)
{
    return (uint8_t*)h + sizeof(Head); // + h->size;
}

static Head * tail_head(Tail * t)
{
    return (Head *)((uint8_t *)t - (sizeof(Head) + t->size));
}

void halloc_integrity_check(const Halloc * cntx)
{
#if VALIDATE
    Head * h = cntx->start;
    while (h < (Head *)cntx->end) {
        Tail * t = head_tail(h);
        ASSERT(h->magic == HEAD_MAGIC);
        ASSERT(h->size);
        ASSERT(t->magic == TAIL_MAGIC);
        ASSERT(t->size);
        ASSERT(h->size == t->size);
        h = head_next(h);
    }
    ASSERT(h == cntx->end);
#endif
}

void halloc_init(Halloc * cntx, void * start, unsigned int initial_size)
{
    if (initial_size <= sizeof(Head) + sizeof(Tail)) {
        int extra = 0x1000;
        cntx->grow_cb(cntx, extra);
        initial_size += extra;
    }

#if VALIDATE
    memset(start, 0xFA, initial_size);
#endif

    Head * h = start;
#if VALIDATE
    h->magic = HEAD_MAGIC;
#endif
    h->size  = initial_size - (sizeof(Head) + sizeof(Tail));
    h->used  = 0;

    Tail * t = head_tail(h);
#if VALIDATE
    t->magic = TAIL_MAGIC;
#endif
    t->size  = h->size;

    cntx->start = h;
    cntx->end   = t + 1;
}

static uintptr_t calc_alignment_size(const Head * h, uintptr_t size, int page_align)
{
    uintptr_t data0 = (uintptr_t)(h + 1);
    uintptr_t data1 = data0;
    if (page_align) {
        data1 += sizeof(Head) + 1 + sizeof(Tail);
        data1 += (page_align - 1);
        data1 &= ~(page_align - 1);
    }
    return (data1 - data0) + size;  /* padding plus size */
}

static int grow(Halloc * cntx, uintptr_t size, int page_align, Head ** head_ptr)
{
    /* ...[hl]...[tl]         */
    /*              |<-- end */

    Tail * tl = (Tail *)cntx->end - 1;
#if VALIDATE
    ASSERT(tl->magic == TAIL_MAGIC);
#endif
    Head * hl = tail_head(tl);
#if VALIDATE
    ASSERT(hl->magic == HEAD_MAGIC);
#endif

    size_t extra;

    if (!hl->used) {
        extra = calc_alignment_size(hl, size, page_align);
        if (hl->size < extra)
            extra -= hl->size;
    } else {
        /* increase size to accomodate any page alignment padding */
        size = calc_alignment_size(cntx->end, size, page_align);

        /* how much extra memory do we need (in worst case, we'll need to add head and tail) */
        extra = sizeof(Head) + size + sizeof(Tail);
    }

    /* align to nearest 4k page */
    extra += 0xfff;
    extra &= ~0xfff;

    Head * hn = cntx->end;

    if (cntx->grow_cb) {
        int ret = cntx->grow_cb(cntx, extra);
        if (ret < 0)
            return ret;
    }

    if (!hl->used) {
        /* from: ...[hl]...[tl]NNNNNNN */
        /*   to: ...[hl]..........[tn] */
        hl->size += extra;

        Tail *tn = head_tail(hl);
        ASSERT(tn == (Tail*)cntx->end - 1);
#if VALIDATE
        tn->magic = TAIL_MAGIC;
#endif
        tn->size = hl->size;

        /* nuke tl */
#if VALIDATE
        tl->magic = 0;
#endif

        *head_ptr = hl;
        return 0;
    } else {
        /* from: ...[hl]...[tl]NNNNNNNNNNN  */
        /*   to: ...[hl]...[tl][hn]...[tn] */
#if VALIDATE
        hn->magic = HEAD_MAGIC;
#endif
        hn->size  = extra - (sizeof(Head) + sizeof(Tail));
        hn->used  = 0;

        Tail * tn = head_tail(hn);
        ASSERT(tn == (Tail*)cntx->end - 1);
#if VALIDATE
        tn->magic = TAIL_MAGIC;
#endif
        tn->size  = hn->size;

        *head_ptr = hn;
        return 0;
    }
}

static Head * find_reserve(Halloc * cntx)
{
    Head * hreserve = NULL;
    for (Head * h = cntx->start; h < (Head *)cntx->end; h = head_next(h)) {
        if (h->used)
            continue;
        int aligned_size = calc_alignment_size(h, cntx->reserve_size, 0);
        if (h->size < aligned_size)
            continue;
        if (!hreserve || h->size < hreserve->size)
            hreserve = h;
    }
    return hreserve;
}

/* if possible, shrink memory usage to nearest page boundary */
static void shrink(Halloc * cntx)
{
    Tail * t = (Tail *)cntx->end - 1;
#if VALIDATE
    ASSERT(t->magic == TAIL_MAGIC);
#endif
    Head * h = tail_head(t);
#if VALIDATE
    ASSERT(h->magic == HEAD_MAGIC);
#endif

    if (h->used || h->size < 8192 || h == find_reserve(cntx))
        return;

    uintptr_t boundary = (((uintptr_t)h + 0x1000) & ~0xfff);
    uintptr_t size = boundary - (uintptr_t)h;
    if (size <= sizeof(Head) + sizeof(Tail))
        return;

    /*  [h].........[t] */
    /*  [h]...[t]ffffff */
    /*           |<- page boundary */

    /* nuke t */
#if VALIDATE
    t->magic = 0;
#endif

    if (cntx->shrink_cb)
        cntx->shrink_cb(cntx, boundary);

    cntx->end = (Head *)boundary;

    h->size  = size - (sizeof(Head) + sizeof(Tail));

    t = (Tail *)cntx->end - 1;
#if VALIDATE
    t->magic = TAIL_MAGIC;
#endif
    t->size  = h->size;

    ASSERT(head_tail(h) == t);
}

#define log2int(x) (31 - __builtin_clz(x))

void * halloc(Halloc * cntx, const unsigned int size, int page_align, int use_reserve, const char * tag)
{
repeat:
    halloc_integrity_check(cntx);

    if (!size || size > INT_MAX/2)
        return NULL;

    /* find smallest hole fitting reserve size */
    Head * hreserve = NULL;
    if (cntx->reserve_size && !use_reserve) {
        hreserve = find_reserve(cntx);
    }

    /* find smallest hole */
    Head * h0 = NULL;
    for (Head * h = cntx->start; h < (Head *)cntx->end; h = head_next(h)) {
        if (h->used || h == hreserve)
            continue;
        int aligned_size = calc_alignment_size(h, size, page_align);
        if (h->size < aligned_size)
            continue;
        if (!h0 || h->size < h0->size)
            h0 = h;
    }

    if (use_reserve)
        ASSERT(h0);

    if (!use_reserve && (!h0 || !hreserve)) {
        int ret = grow(cntx, size + (!hreserve ? cntx->reserve_size : 0), page_align, &h0);
        if (ret == -ENOMEM)
            return NULL;
        if (ret == -EAGAIN)
            goto repeat;
    }

    /* accomodate page alignment padding */

    uintptr_t data0 = (uintptr_t)head_data(h0);
    uintptr_t data1 = data0;
    if (page_align) {
        data1 += sizeof(Head) + 1 + sizeof(Tail);
        data1 += (page_align - 1);
        data1 &= ~(page_align - 1);
    }

    if (data0 != data1) { /* alignment required */

        /* from: [h0]..............[t1] */
        /*   to: [h0]...[t0][h1]xxx[t1] */
        /*                     |<- page boundary */

        Tail * t1 = head_tail(h0);
#if VALIDATE
        ASSERT(t1->magic == TAIL_MAGIC);
#endif

        int orig_size = h0->size;

        h0->size = (data1 - data0) - (sizeof(Head) + sizeof(Tail));

        Tail * t0 = head_tail(h0);
#if VALIDATE
        t0->magic = TAIL_MAGIC;
#endif
        t0->size  = h0->size;

        Head * h1 = (Head *)data1 - 1;
#if VALIDATE
        h1->magic = HEAD_MAGIC;
#endif
        h1->size  = orig_size - h0->size - (sizeof(Head) + sizeof(Tail));
        h1->used  = 1;
        ASSERT(head_tail(h1) == t1);

        t1->size  = h1->size;

        h0 = h1;
    }

    /* if the h0 candidate block is large, split it */
    if (h0->size - size > sizeof(Head) + sizeof(Tail) + 0) {

        /* from:  [h0]..............[t1]  */
        /*   to:  [h0]xxx[t0][h1]...[t1] */

        /* nuke the original tail, for sanity checking */
        Tail * t1 = head_tail(h0);
#if VALIDATE
        ASSERT(t1->magic == TAIL_MAGIC);
        t1->magic = 0;
#endif

        /* adjust h0 */
        int original_size = h0->size;
        h0->size  = size;

        /* install t0 */
        Tail * t0 = head_tail(h0);
#if VALIDATE
        t0->magic = TAIL_MAGIC;
#endif
        t0->size  = size;

        /* install h1 */
        Head * h1 = head_next(h0);
#if VALIDATE
        t0->magic = TAIL_MAGIC;
        h1->magic = HEAD_MAGIC;
#endif
        h1->size  = original_size - (size + sizeof(Head) + sizeof(Tail));
        h1->used  = 0;

        ASSERT(head_tail(h1) == t1);

#if VALIDATE
        t1->magic = TAIL_MAGIC;
#endif
        t1->size  = h1->size;
    }

    h0->align = page_align ? log2int(page_align) : 0;
    h0->used = 1;

#if VALIDATE
    strlcpy(h0->tag, tag, sizeof(h0->tag));

    memset(head_data(h0), 0xAB, size);
    if (page_align)
        ASSERT(((uintptr_t)head_data(h0) & (page_align - 1)) == 0);
#endif

#if 0
    cntx->printf("halloc(%d, %d) = %p\n", size, page_align, head_data(h0));
    if (cntx->dump_cb) cntx->dump_cb(cntx);
    halloc_dump2(cntx, head_data(h0), "RECENT HALLOC");
#endif
    halloc_integrity_check(cntx);

    return head_data(h0);
}

void hfree(Halloc * cntx, void * ptr)
{
    if (!ptr)
        return;

    halloc_integrity_check(cntx);

    Head * h = (Head *)ptr - 1;//((uint8_t *)ptr - sizeof(Head));
#if VALIDATE
    ASSERT(h->magic == HEAD_MAGIC);
#endif
    ASSERT(h->used);

    h->used = 0;

    /* [hl]....[tl][h]...[t][hr]...[tr] */

    /* try merge right */
    Head *hr = head_next(h);
    if (hr < (Head *)cntx->end && !hr->used) {
        ASSERT(hr->magic == HEAD_MAGIC);

#if VALIDATE
        /* nuke t */
        Tail * t = head_tail(h);
        t->magic = 0;
#endif

        h->size += hr->size + sizeof(Head) + sizeof(Tail);
        Tail * tr = head_tail(hr);
#if VALIDATE
        ASSERT(tr->magic == TAIL_MAGIC);
#endif
        tr->size = h->size;

#if VALIDATE
        /* nuke hr */
        hr->magic = 0;
#endif
    }

    Tail * tl = (Tail*)((uint8_t*)h - sizeof(Tail));
    if (tl > (Tail *)cntx->start) {
#if VALIDATE
        ASSERT(tl->magic == TAIL_MAGIC);
#endif
        Head * hl = tail_head(tl);
        ASSERT(hl->magic == HEAD_MAGIC);
        if (!hl->used) {
#if VALIDATE
            /* nuke tl */
            tl->magic = 0;
#endif

            hl->size += h->size + sizeof(Head) + sizeof(Tail);
            Tail * t = head_tail(hl);
            t->size = hl->size;

#if VALIDATE
            /* nuke h */
            h->magic = 0;
#endif
        }
    }

    shrink(cntx);

    halloc_integrity_check(cntx);
}

void * hrealloc(Halloc * cntx, void * ptr, unsigned int size)
{
    halloc_integrity_check(cntx);

    if (!ptr)
        return halloc(cntx, size, 0, 0, "hrealloc");

    Head * h = (Head *)ptr - 1;//((uint8_t *)ptr - sizeof(Head));
    ASSERT(h->magic == HEAD_MAGIC);
    ASSERT(h->used);

    if (size <= h->size)
        return ptr;

    //FIXME: under any circumstance do we split blocks

    //FIXME: if next block is empty, and that is enough space, merge into that block

    void * tmp = halloc(cntx, size, h->align ? 1 << h->align : 0, 0,
#if VALIDATE
        h->tag
#else
        NULL
#endif
        );
    if (!tmp)
        return NULL;
    memcpy(tmp, ptr, h->size);
    hfree(cntx, ptr);

    halloc_integrity_check(cntx);
    return tmp;
}

void halloc_dump2(const Halloc * cntx, void * highlight, const char * name)
{
#if VALIDATE
    Head * h = cntx->start;
    while (h < (Head *)cntx->end) {
        Tail * t = head_tail(h);
        cntx->printf("\t[H:size=%5d, used=%d] ... [T:size=%5d] %p '%s' %s%s\n", (int)h->size, (int)h->used, (int)t->size, head_data(h), h->used ? h->tag : "",
            head_data(h) == highlight ? " <--- " : "",
            head_data(h) == highlight ? name : "");
        ASSERT(h->magic == HEAD_MAGIC);
        ASSERT(h->size);
        ASSERT(t->magic == TAIL_MAGIC);
        ASSERT(t->size);
        ASSERT(h->size == t->size);

        h = head_next(h);
    }
    cntx->printf("\tend=%p\n\n", cntx->end);
#endif
}

void halloc_dump(const Halloc * cntx)
{
    halloc_dump2(cntx, NULL, NULL);
}

#if TEST
static int check(unsigned char * ptr, int v, unsigned int size)
{
    for (unsigned int i = 0; i < size; i++)
        if (ptr[i] != v)
            return -1;
    return 0;
}

static int grow_cb(Halloc * cntx, unsigned int extra)
{
    Head * hend_new = (Head *)((uint8_t*)cntx->end + extra); /* magic */
    if ((uint8_t *)hend_new - (uint8_t *)cntx->start > MEMORY_SIZE)
        return -ENOMEM;
    cntx->end = hend_new;
    return 0;
}

int main()
{
    void * store = malloc(MEMORY_SIZE);

    Halloc cntx;
    halloc_init(&cntx, store, 8192*2);
    cntx.reserve_size = 8192;
    cntx.grow_cb = grow_cb;
    cntx.shrink_cb = 0;
    cntx.dump_cb = 0;
    cntx.printf = printf;
    halloc_dump(&cntx);

#if 1
    /* grinder */

#define NB 256
    unsigned char * array[NB] = { 0 };
    unsigned int sizes[NB];
    srand(0);
    for (unsigned int i = 1; i < 1000000; i++) {
        int idx = random() % NB;
 //   halloc_dump();
        if (array[idx]) {
            if (check(array[idx], idx, sizes[idx]) < 0) {
                printf("integrity check failed: %d\n", idx);
                for (unsigned int i = 0; i < sizes[idx]; i++) {
                    printf(" %02x", array[idx][i]);
                }
                exit(1);
            }
            hfree(&cntx, array[idx]);
            array[idx] = NULL;
        } else {
            sizes[idx] = random() % 10000;
            array[idx] = halloc(&cntx, sizes[idx], random() ? 4 : 4096, 0, "stdlib");
            if (array[idx])
                memset(array[idx], idx, sizes[idx]);
        }
 //    halloc_dump();
    }
    halloc_dump(&cntx);
    printf("PASS\n");
#else
    void *p = halloc(&cntx, 25, 0, "p");
    void *q = halloc(&cntx, 30, 0, "q");
    halloc_dump(&cntx);
    hfree(&cntx, q);
    hfree(&cntx, p);
    halloc_dump(&cntx);
#endif
}
#endif /* TEST */
