#include <stdint.h>
#include <stdarg.h>
#include <stddef.h> //offsetof
#include <string.h>

#include  "dev.h"
#include "tty.h"
#include "ata.h"
#include "ext2.h"
#include "loop.h"
#include "mem.h"
#include "ne2k.h"
#include "pipe.h"
#include "proc.h"
#include "serial.h"
#include "utils.h"
#include "vfs.h"
#include "paging.h"

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <bsd/string.h>
#include <time.h>
#include <limits.h>
#include <stdio.h>
#include <signal.h>

#define SPRAY_MEMORY 0

/* common */

void panic(const char * reason)
{
    kprintf("* * * PANIC * * *: %s\n", reason);

    tty_puts("* * * PANIC * * *: ");
    tty_puts(reason);
    tty_puts("\n");

    asm volatile("cli");
    for (;;) ;
}

static void kputc(__attribute((unused)) void *cntx, int c)
{
    outb(0xE9, c);
}

#include "generic_printf.h"

int kprintf(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    generic_vprintf(kputc, NULL, fmt, args);
    va_end(args);
    return 0; //FIXME: return bytes printed
}

/* multiboot */

#include "multiboot.h"

void khexdump(const void * ptr, unsigned int size)
{
     const uint8_t * buf = ptr;
     for (unsigned i = 0 ; i < size; i += 16) {
         kprintf("%5x:", i);
         for (unsigned int j = i; j < size && j < i + 16; j++) {
             kprintf(" %02x", buf[j]);
         }
         kprintf(" | ");
         for (unsigned int j = i; j < size && j < i + 16; j++) {
             kprintf("%c", buf[j] >= 32 && buf[j] <= 127 ? buf[j] : '.');
         }
         kprintf("\n");
     }
}

extern uint32_t multiboot;
extern uint32_t end;
char cmdline[64];

static const char * get_cmdline_token(const char * token, const char * def, char * out, size_t out_size)
{
    char * s = strstr(cmdline, token);
    if (!s) {
        strlcpy(out, def, out_size);
        return out;
    }
    s += strlen(token);
    char *p = s;
    while (*p && *p != ' ')
        p++;
    snprintf(out, out_size, "%.*s", (int)(p - s), s);
    return out;
}

static void parse_multiboot_info(const multiboot_info * info, uint32_t * low_size, uint32_t * up_start, uint32_t * up_size, uint32_t * mod_start, uint32_t * mod_end)
{
    *up_start = (uint32_t)&end;

    if (info->flags & MULTIBOOT_INFO_CMDLINE) {
        kprintf("cmdline: %s\n", (const char *)info->cmdline);
        strlcpy(cmdline, (const char *)info->cmdline, sizeof(cmdline));
    }

    /* do this first */
    if (info->flags & MULTIBOOT_INFO_FRAMEBUFFER) {
        if (info->framebuffer_type <= 1) {
            fb_init(info->framebuffer_addr, info->framebuffer_pitch, info->framebuffer_width, info->framebuffer_height, info->framebuffer_bpp);
            tty = &fb_commands;
        }
        kprintf("framebuffer: addr=0x%llx, pitch=0x%x, width=%d, height=%d, bpp=%d, type=%d\n", info->framebuffer_addr, info->framebuffer_pitch, info->framebuffer_width, info->framebuffer_height, info->framebuffer_bpp, info->framebuffer_type);
    }
    if (tty == &textmode_commands)
        textmode_init();

    kb_init();
    tty_init();

    if (info->flags & MULTIBOOT_INFO_BOOTDEV) {
        kprintf("boot_device: 0x%x\n", info->boot_device);
    }
    if (info->flags & MULTIBOOT_INFO_MODS) {
        kprintf("mods_count: %d\n", info->mods_count);
        const multiboot_mod * mod = (const multiboot_mod *)info->mods_addr;
        for (unsigned int i = 0; i < info->mods_count; i++) {
            kprintf("mod[%d] mod_start 0x%x, mod_end 0x%x, mod_name '%s'\n", i, mod[i].mod_start, mod[i].mod_end, mod[i].name);
            khexdump((void *)mod[i].mod_start, MIN(mod[i].mod_end - mod[i].mod_start, 32));

            *mod_start = mod[i].mod_start;
            *mod_end = mod[i].mod_end;
            *up_start = MAX(*up_start, mod[i].mod_end);
        }
    }

    if (info->flags & MULTIBOOT_INFO_MEMORY) { /* do this after finding greatest module */
        kprintf("mem_lower 0x%x, mem_higher 0x%x\n", info->mem_lower, info->mem_higher);
        *low_size = info->mem_lower * 1024;
        *up_size = info->mem_higher * 1024 - (*up_start - (uint32_t)&multiboot);
    } else {
        panic("no memory information\n");
        *low_size = 0;
        *up_size = 0;
    }

    if (info->flags & MULTIBOOT_INFO_MMAP) {
        //kprintf("mmap: length:0x%x, addr:0x%x\n", info->mmap_length, info->mmap_addr);
        for (unsigned int i = 0; i < info->mmap_length; ) {
            const multiboot_memory_map * mmap = (void*)(info->mmap_addr + i);
            //FIXME: assert mmap->size >= 0x14
            kprintf("mmap: size:0x%x, base_addr:0x%llx, length:0x%llx, type:%s\n", mmap->size, mmap->base_addr, mmap->length, mmap->type == 1 ? "available" : "reserved");
            i += mmap->size + 4;
        }
    }
    /*
    if (info->flags & MULTIBOOT_INFO_VBE) {
        // this only appears on device with legacy bios, probably not worth investigating
        kprintf("VBE control_info:0x%x, mode_info:0x%x", info->vbe_control_info, info->vbe_mode_info);
        kprintf("VBE interface seg:0x%x, off:0x%x, len:0x%x\n", info->vbe_interface_seg, info->vbe_interface_off, info->vbe_interface_len);
    }
    */
}

#define read_8(params, offset) params[offset]
#define read_16(params, offset) *(uint16_t *)(params + offset)
#define read_32(params, offset) *(uint32_t *)(params + offset)
static void parse_linux_params(const uint8_t * params, uint32_t * low_size, uint32_t * up_start, uint32_t * up_size, uint32_t * mod_start, uint32_t * mod_end)
{
    *low_size = 639*1024;
    *up_start = (uint32_t)&end;

    kprintf("cmdline: %s\n", read_32(params, 0x228));
    strlcpy(cmdline, (const char *)read_32(params, 0x228), sizeof(cmdline));

    if (read_8(params, 0xf) == 0x23) {
        fb_init(/*base*/read_32(params, 0x18), /*stride*/read_16(params,0x24), /*width*/read_16(params, 0x12), /*height*/read_16(params, 0x14), /*depth*/read_16(params, 0x16));
        tty = &fb_commands;
    } else
        textmode_init();

    kb_init();
    tty_init();

    void * initrd_start = (void *)read_32(params, 0x218);
    uint32_t initrd_size = read_32(params, 0x21c);
    /* move initrd to immediately after kernel image */
    if (initrd_size) {
        memcpy((void *)*up_start, initrd_start, initrd_size);
        *mod_start = *up_start;
        *mod_end = *mod_start + initrd_size;

       *up_start += initrd_size;
    }

    uint32_t alt_mem_k = read_32(params, 0x1e0);
    *up_size = (alt_mem_k * 1024) - (*up_start - (uint32_t)&multiboot);
}

/* cpu */

static word get_cr0()
{
    word cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

static void set_cr0(word cr0)
{
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

static void * get_cr2()
{
    void * cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

static word get_cr4()
{
    word cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    return cr4;
}

static void set_cr4(word cr4)
{
    asm volatile("mov %0, %%cr4" : : "r"(cr4));
}

static inline uint32_t rdmsr(int msr)
{
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static void wrmsr(int msr, uint64_t v)
{
    uint32_t low  = v & 0xFFFFFFFF;
    uint32_t high = v >> 32;
    asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

static int cpu_has_pat = 0;
static void cpu_init()
{
    uint32_t eax, ebx, ecx, edx;
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));

    if (edx & (1 << 16)) { //PAT
        uint64_t pat = rdmsr(0x277);
        pat &= ~(7ULL << 32);
        pat |=   1ULL << 32; /* clear and set PA4 to 0x1 Write Combining (WC) */
        wrmsr(0x277, pat);
        cpu_has_pat = 1;
    }

    if (edx & (1 << 25)) { //SSE
        uint32_t cr0 = get_cr0();
        cr0 &= ~4; // EM
        cr0 |= 2; // MP
        set_cr0(cr0);

        uint32_t cr4 = get_cr4();
        cr4 |= (1<<9); // OSFXSR
        cr4 |= (1<<10); // OSXMMEXCPT
        set_cr4(cr4);
    }
}

/* user space */

typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} tss_entry_struct;

static tss_entry_struct tss_entry;

static void set_kernel_stack(uint32_t stack)
{
    tss_entry.esp0 = stack;
}

/* gdt */

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t flags;
    uint8_t granularity;
    uint8_t base_high;
} __attribute((packed)) gdt_entry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute((packed)) gdt_pointer;

typedef struct {
   uint16_t base_low;
   uint16_t selector;
   uint8_t zero;
   uint8_t flags;
   uint16_t base_high;
} __attribute((packed)) idt_entry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute((packed)) idt_pointer;

static gdt_entry gdt[6];
static gdt_pointer gdt_ptr;
static idt_entry idt[256];
static idt_pointer idt_ptr;

void gdt_flush(gdt_pointer *);
void idt_flush(idt_pointer *);
void tss_flush(void);

static void gdt_set(int idx, uint32_t base, uint32_t limit, uint8_t flags, uint8_t granularity)
{
    gdt[idx].base_low    = base & 0xFFFF;
    gdt[idx].base_middle = (base >> 16) & 0xFF;
    gdt[idx].base_high   = base >> 24;

    gdt[idx].limit_low   = limit & 0xFFFF;
    gdt[idx].granularity = ((limit >> 16) & 0xF) | granularity;

    gdt[idx].flags       = flags;
}

static void idt_set(int idx, uint32_t base, uint16_t selector, uint8_t flags)
{
    idt[idx].base_low  = base & 0xFFFF;
    idt[idx].base_high = base >> 16;
    idt[idx].selector  = selector;
    idt[idx].zero      = 0;
    idt[idx].flags     = flags;
}

static void tss_set(int idx, uint16_t ss0, uint32_t esp0)
{
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = base + sizeof(tss_entry);

    gdt_set(idx, base, limit, 0xE9, 0x00);

    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0 = ss0; // kernel stack segment
    tss_entry.esp0 = esp0; // kernel stack pointer

    tss_entry.cs = 0x0b; // kernel code segment (0x08) + 'requested privellege level' (3)
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;  //kernel data segment (0x10) + requested privilege level (3)
}

static void init_gdt()
{
#define G_4096 (1<<7)
#define G_32BIT (1<<6)
#define G_64BIT (1<<5)
           /* base  limit       flags granularity */
    gdt_set(0, 0x0, 0x0,        0x0,  0x00);
    gdt_set(1, 0x0, 0xFFFFFFFF, 0x9A, G_4096 | G_32BIT); /* 0x8 kernel code */
    gdt_set(2, 0x0, 0xFFFFFFFF, 0x92, G_4096 | G_32BIT); /* 0x10 kernel data */
    gdt_set(3, 0x0, 0xFFFFFFFF, 0xFA, G_4096 | G_32BIT); /* 0x18 user code */
    gdt_set(4, 0x0, 0xFFFFFFFF, 0xF2, G_4096 | G_32BIT); /* 0x20 user data */
    tss_set(5, 0x10, 0x0);

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_flush(&gdt_ptr);
    tss_flush();
}

typedef struct
{
    uint32_t ds;
    uint32_t edi, esi, ebp, ebx, edx, ecx, eax;
    uint32_t number;
    uint32_t error_code;
    uint32_t eip, cs, eflags, esp, ss;
} registers;

typedef struct StackFrame StackFrame;
struct StackFrame {
    StackFrame * ebp;
    uint32_t eip;
};

static void dump_registers(const registers * regs)
{
    kprintf("    EIP=0x%x, ESP=0x%x, CS=0x%x, DS=0x%x, SS=0x%x\n", regs->eip, regs->esp, regs->cs, regs->ds, regs->ss);
    kprintf("    EAX=0x%x, EBX=0x%x, ECX=0x%x, EDX=0x%x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    kprintf("    ESI=0x%x, EDI=0x%x\n", regs->esi, regs->edi);
    kprintf("    EFLAGS=0x%x\n", regs->eflags);

    StackFrame * s = (StackFrame *)regs->ebp;
    for(int i = 0; s > (StackFrame *)0x14 && i < 16; i++) {
        kprintf("Stack EIP: %p\n", s->eip);
        s = s->ebp;
    }
}

/* task */

enum {
    STATE_RUNNING = 0,
    STATE_STOPPED,
    STATE_ZOMBIE,
    STATE_WAITPID,
    STATE_NANOSLEEP,
    STATE_READ,
    STATE_WRITE,
    STATE_PTHREAD_JOIN,
    STATE_SELECT,
    STATE_PAUSE,
    STATE_ACCEPT,
    NB_STATES
};

static const char * state_names[NB_STATES] = {
    [STATE_RUNNING] = "running",
    [STATE_STOPPED] = "stopped",
    [STATE_ZOMBIE] = "zombie",
    [STATE_WAITPID] = "waitpid",
    [STATE_NANOSLEEP] = "nanosleep",
    [STATE_READ] = "read",
    [STATE_WRITE] = "write",
    [STATE_PTHREAD_JOIN] = "pthread_join",
    [STATE_SELECT] = "select",
    [STATE_PAUSE] = "pause",
    [STATE_ACCEPT] = "accept",
};

typedef struct Join Join;
struct Join {
    pthread_t thread;
    void * value;
    Join * next;
};

typedef struct {
    int pgrp; /* process group number */
    page_directory * page_directory;
    FileDescriptor * fd[OPEN_MAX];
    uint32_t brk;
    char cwd[255];
    uint32_t stack_next;
    struct itimerval timer;
    int alarm_armed;
    struct timespec alarm;
    int errorlevel;

    sigset_t signal;
    struct sigaction act[NSIG];
    int thread_signal;

    Join * joins; /* list of terminated threads */

} Process;

typedef struct Task Task;
struct Task {
    char name[255];
    int id;
    int ppid; /* parent pid */
    int state;

    registers reg;
    uint32_t stack_top;

    union {
        struct {
            int parent_notified;
        } stopped;
        struct {
            int * status; //userspace pointer
        } waitpid;
        struct {
            struct timespec expire;
        } nanosleep;
        struct {
            pthread_t thread;
            void ** value_ptr; //userspace pointer
        } pthread_join;
        struct {
            FileDescriptor * fd;
            uint8_t * buf; //userspace pointer
            size_t size;
            int pos;
        } read;
        struct {
            FileDescriptor * fd;
            const uint8_t * buf; //userspace pointer
            size_t size;
            int pos;
        } write;
        struct {
            int nfds;
            fd_set * readfds; //userspace pointer
            fd_set * writefds; //userspace pointer
            fd_set * errorfds; //userspace pointer
            struct timespec expire;
        } select;
        struct {
            FileDescriptor * fd;
        } accept;
    } u;

    Process * proc;

    Task * thread_parent;
    Task * next;
};

static Task * current_task = NULL;
static Task * ready_queue;
static Task * wait_queue =  NULL;
static Task * zombie_queue = NULL;

static unsigned int next_pid = 1;

FileDescriptor ** get_current_task_fds(void);
FileDescriptor ** get_current_task_fds()
{
    return current_task->proc->fd;
}

#include <os/syscall.h>
#include <errno.h>

static void switch_task(registers * reg);
static int sys_fork(const registers * reg);
static int sys_getpid(void);
static int sys_getppid(void);
static int exit2(int status, int thread_termination);
static int sys_exit(int status);
static int sys_write(registers * reg, int fd, const void * buf, size_t size);
static int sys_read(registers * reg, int fd, void * buf, size_t size);
static int sys_open(const char * path, int flags, mode_t mode);
static int sys_close(int fd);
static off_t sys_lseek(int fd, off_t offset, int whence);
static int sys_getdents(int fd, struct dirent * de, size_t count);
static int sys_dup(int fd);
static int sys_dup2(int fd, int fd2);
static int sys_unlink(const char *path);
static int sys_rmdir(const char *path);
static int sys_mkdir(const char *path, mode_t mode);
static int sys_brk(uint32_t addr, uint32_t * current_brk);
static int sys_execve(registers * regs, const char * pathname, char * const argv[], char * const envp[]);
static pid_t sys_waitpid(registers * reg, pid_t pid, int * stat_loc, int options);
static int sys_getcwd(char * buf, size_t size);
static int sys_chdir(const char * path);
static int sys_stat(const char * path, struct stat * st);
static int sys_lstat(const char * path, struct stat * buf);
static int sys_fstat(int fd, struct stat * st);
static int sys_clock_gettime(clockid_t clock_id, struct timespec *tp);
static int sys_nanosleep(registers * reg, const struct timespec * ts, struct timespec * rem);
static int sys_setpgid(pid_t pid, pid_t pgid);
static pid_t sys_getpgrp(void);
static pid_t sys_setpgrp(void);
static pid_t sys_tcgetpgrp(int fildes);
static int sys_tcsetpgrp(int fd, pid_t pgrp);
static int sys_kill(pid_t pid, int sig);
static int sys_uname(struct utsname * name);
static int sys_ioctl(int fd, int request, void * data);
static int sys_mmap(struct os_mmap_request * req);
static int sys_fcntl(int fd, int cmd, int value);
static int sys_tcgetattr(int, struct termios *);
static int sys_tcsetattr(int, int, const struct termios *);
static int sys_pthread_create(pthread_t * thread, const pthread_attr_t * attr, void * start_routine, void * arg);
static int sys_pthread_join(registers * reg, pthread_t thread, void ** value_ptr);
static int sys_sigaction(int sig, const struct sigaction * act, struct sigaction * oact);
static int sys_getitimer(int which, struct itimerval * value);
static int sys_setitimer(int which, const struct itimerval * value, struct itimerval * ovalue);
static int sys_select(registers * reg, int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds, struct timeval * timeout);
static int sys_pipe(int * fildes);
static int sys_isatty(int fildes);
static int sys_syslog(int priority, const char * message);
static int sys_utime(const char * path, const struct utimbuf * times);
static int sys_rename(const char * old, const char * new);
static int sys_pause(registers * reg);
static unsigned sys_alarm(registers * reg, unsigned seconds);
static int sys_getmntinfo(struct statvfs * mntbufp, int size);
ssize_t sys_readlink(const char * path, char * buf, size_t bufsize);
int sys_symlink(const char * path1, const char * path2);
int sys_link(const char * path1, const char * path2);
int sys_ftruncate(int fildes, off_t length);
int sys_fstatat(int fd, const char * path, struct stat * buf, int flag);
int sys_fchdir(int fildes);
static pid_t sys_getpgid(pid_t pid);
int sys_socket(int domain, int type, int protocol);
int sys_bind(int socket, const struct sockaddr * address, socklen_t address_len);
int sys_accept(registers * reg, int socket, struct sockaddr * address, socklen_t * address_len);
int sys_connect(int socket, const struct sockaddr * address, socklen_t address_len);

#define TICKS_PER_SECOND 100
/* monotonic counter */
struct timespec tnow = {.tv_sec = 0, .tv_nsec = 0};

static int cmos_read(int reg)
{
    outb(0x70, reg);
    return inb(0x71);
}

#include <time.h>
static void rtc_init()
{
    while(cmos_read(0xA) & 0x80) ;

    struct tm tm;
    tm.tm_year = cmos_read(0x09);
    tm.tm_mon = cmos_read(0x08);
    tm.tm_mday = cmos_read(0x07);
    tm.tm_wday = cmos_read(0x06);
    tm.tm_hour = cmos_read(0x04);
    tm.tm_min = cmos_read(0x02);
    tm.tm_sec = cmos_read(0x00);
    int century = cmos_read(0x32);
    int register_b = cmos_read(0x0B);

    if (!(register_b & 0x04)) {
#define BCD_TO_BIN(bcd) ( ( (bcd & 0xF0) >> 1) + ( (bcd & 0xF0) >> 3) + (bcd & 0xF))
        tm.tm_sec = BCD_TO_BIN(tm.tm_sec);
        tm.tm_min = BCD_TO_BIN(tm.tm_min);
        tm.tm_hour = BCD_TO_BIN(tm.tm_hour);
        tm.tm_mday = BCD_TO_BIN(tm.tm_mday);
        tm.tm_wday = BCD_TO_BIN(tm.tm_wday);
        tm.tm_mon = BCD_TO_BIN(tm.tm_mon);
        tm.tm_year = BCD_TO_BIN(tm.tm_year);
        century = BCD_TO_BIN(century);
    }

    tm.tm_year += century * 100;

    /* adjust for struct tm ranges */
    tm.tm_year -= 1900;
    tm.tm_mon  -= 1;
    tm.tm_wday -= 1;

    kprintf("rtc_init: %s\n", asctime(&tm));

    tnow.tv_sec = mktime(&tm);

    /* reconstruct, should match earlier asctime */
    kprintf("rtc_init: %s\n", ctime(&tnow.tv_sec));
}

static void update_itimers_task(Task * t)
{
    struct itimerval * it = &t->proc->timer;
    if (it->it_interval.tv_sec || it->it_interval.tv_usec) {
        it->it_value.tv_usec -= 1000000 / TICKS_PER_SECOND;
        if (it->it_value.tv_usec < 0) {
            it->it_value.tv_usec = 1000000;
            it->it_value.tv_sec--;
            if (it->it_value.tv_sec < 0) {
                it->it_value.tv_sec  = it->it_interval.tv_sec;
                it->it_value.tv_usec = it->it_interval.tv_usec;
                sigaddset(&t->proc->signal, SIGALRM);
            }
        }
    }

    if (t->proc->alarm_armed)  {
        struct timespec * ts = &t->proc->alarm;
        ts->tv_nsec -= 1000000000 / TICKS_PER_SECOND;
        if (ts->tv_nsec < 0) {
            ts->tv_nsec = 1000000000;
            ts->tv_sec--;
            if (ts->tv_sec < 0) {
                ts->tv_sec = 0;
                ts->tv_nsec = 0;
                t->proc->alarm_armed = 0;
                sigaddset(&t->proc->signal, SIGALRM);
            }
        }
    }
}

static void update_itimers()
{
    for (Task * t = ready_queue; t; t = t->next)
        update_itimers_task(t);
    for (Task * t = wait_queue; t; t = t->next)
        update_itimers_task(t);
}

static int do_signal_action(int i);
void * irq_context[16] = {0};
IrqCb * irq_handler[16] = {0};

void interrupt_handler(registers * regs);
void interrupt_handler(registers * regs)
{
    if (regs->number < 32) { /* exceptions */

        if (regs->number == 13) {
            kprintf("GPF: errorcode 0x%x\n", regs->error_code);
            kprintf(" E=%d, Tbl=%d, index=0x%x\n", !!(regs->error_code & 1), (regs->error_code >> 1) & 3, regs->error_code >> 4);
        } else if (regs->number == 14) {
            void * cr2 = get_cr2();
            if (cr2 == (void *)(uint32_t)sys_pthread_join && current_task->thread_parent) {
                if (current_task->proc->thread_signal) {
                    Task * parent = current_task->thread_parent;
                    exit2(1, 1);

                    current_task = parent; /* okay */
                    do_signal_action(current_task->proc->thread_signal);

                    current_task = NULL;

                    switch_task(regs);
                    return;
                }

                Join * j = kmalloc(sizeof(Join), "join");
                j->thread = current_task->id;
                j->value = (void *)regs->eax;
                j->next = current_task->proc->joins;
                current_task->proc->joins = j;

                exit2(1, 1);
                switch_task(regs);
                return;
            }
            kprintf("PAGE FAULT: address 0x%x, error_code 0x%x:", cr2, regs->error_code);
            if (regs->error_code & 1)  kprintf(" Present"); else kprintf(" Not-Present");
            if (regs->error_code & 2)  kprintf(" Write"); else kprintf(" Read");
            if (regs->error_code & 4)  kprintf(" User"); else kprintf(" Supervisor");
            if (regs->error_code & 8)  kprintf(" Reserved-Write");
            if (regs->error_code & 16)  kprintf(" Instruction-Fetch");
            kprintf("\n");
            tty_puts("Segmentation fault\n");
        } else {
            kprintf("OTHER EXCEPTION (%d)\n", regs->number);
        }

        kprintf("current task pid %d %s\n", current_task->id, current_task->name);
        dump_registers(regs);

        if (regs->error_code & 4) {
            sys_exit(1);
            switch_task(regs);
            return;
        }
        panic("unexpected");

    } else if (regs->number < 48) { /* irq */

        int number = regs->number;  /* save the number because switch_task() may blow it away */

        if (number == 32 + 0) {  /* system timer */
            tnow.tv_nsec += 1000000000 / TICKS_PER_SECOND;
            if (tnow.tv_nsec > 1000000000) {
                tnow.tv_sec++;
                tnow.tv_nsec = 0;
            }
            update_itimers();
            switch_task(regs);
        } else {
            int irq = regs->number - 32;
#if 0
            /* this is noisey on laptop */
            kprintf("got irq line %d\n", irq);
#endif
            if (irq_handler[irq])
                irq_handler[irq](irq_context[irq]);
        }

        if (number >= 32 && number < 48) { // reset apic
            if (number >= 40)
                outb(0xA0, 0x20); /* reset slave */
            outb(0x20, 0x20); /* reset master */
        }

    } else if (regs->number == 128) {  // syscall
        int syscall = regs->eax;
        switch (syscall) {
#define SYSCALL0(syscall, name) \
    case syscall: regs->eax = name(); break;
#define SYSCALL0R(syscall, name) \
    case syscall: regs->eax = name(regs); break;
#define SYSCALL1(syscall, name, type1) \
    case syscall: regs->eax = name((type1)regs->ebx); break;
#define SYSCALL1R(syscall, name, type1) \
    case syscall: regs->eax = name(regs, (type1)regs->ebx); break;
#define SYSCALL2(syscall, name, type1, type2) \
    case syscall: regs->eax = name((type1)regs->ebx, (type2)regs->ecx); break;
#define SYSCALL2R(syscall, name, type1, type2) \
    case syscall: regs->eax = name(regs, (type1)regs->ebx, (type2)regs->ecx); break;
#define SYSCALL3(syscall, name, type1, type2, type3) \
    case syscall: regs->eax = name((type1)regs->ebx, (type2)regs->ecx, (type3)regs->edx); break;
#define SYSCALL3R(syscall, name, type1, type2, type3) \
    case syscall: regs->eax = name(regs, (type1)regs->ebx, (type2)regs->ecx, (type3)regs->edx); break;
#define SYSCALL4(syscall, name, type1, type2, type3, type4) \
    case syscall: regs->eax = name((type1)regs->ebx, (type2)regs->ecx, (type3)regs->edx, (type4)regs->esi); break;
#define SYSCALL5R(syscall, name, type1, type2, type3, type4, type5) \
    case syscall: regs->eax = name(regs, (type1)regs->ebx, (type2)regs->ecx, (type3)regs->edx, (type4)regs->esi, (type5)regs->edi); break;
        SYSCALL0 (OS_GETPID, sys_getpid)
        SYSCALL0 (OS_GETPPID, sys_getppid)
        SYSCALL3R(OS_WRITE, sys_write, int, const void *, size_t)
        SYSCALL3R(OS_READ, sys_read, int, void *, size_t)
        SYSCALL0R(OS_FORK, sys_fork)
        SYSCALL1 (OS_EXIT, sys_exit, int)
        SYSCALL3 (OS_OPEN, sys_open, const char *, int, mode_t)
        SYSCALL1 (OS_CLOSE, sys_close, int)
        SYSCALL3 (OS_LSEEK, sys_lseek, int, off_t, int)
        SYSCALL3 (OS_GETDENTS, sys_getdents, int, struct dirent *, size_t)
        SYSCALL1 (OS_DUP, sys_dup, int)
        SYSCALL2 (OS_DUP2, sys_dup2, int, int)
        SYSCALL2 (OS_BRK, sys_brk, uint32_t, uint32_t *)
        SYSCALL1 (OS_UNLINK, sys_unlink, const char *)
        SYSCALL1 (OS_RMDIR, sys_rmdir, const char *)
        SYSCALL2 (OS_MKDIR, sys_mkdir, const char *, mode_t)
        SYSCALL3R(OS_EXECVE, sys_execve, const char *, char * const *, char * const *)
        SYSCALL3R(OS_WAITPID, sys_waitpid, pid_t, int *, int)
        SYSCALL2 (OS_GETCWD, sys_getcwd, char *, size_t)
        SYSCALL1 (OS_CHDIR, sys_chdir, const char *)
        SYSCALL2 (OS_STAT, sys_stat, const char *, struct stat *)
        SYSCALL2 (OS_LSTAT, sys_lstat, const char *, struct stat *)
        SYSCALL2 (OS_FSTAT, sys_fstat, int, struct stat *)
        SYSCALL2 (OS_CLOCK_GETTIME, sys_clock_gettime, int, struct timespec *)
        SYSCALL2R(OS_NANOSLEEP, sys_nanosleep, const struct timespec *, struct timespec *)
        SYSCALL2 (OS_SETPGID, sys_setpgid, pid_t, pid_t)
        SYSCALL0 (OS_GETPGRP, sys_getpgrp)
        SYSCALL0 (OS_SETPGRP, sys_setpgrp)
        SYSCALL1 (OS_TCGETPGRP, sys_tcgetpgrp, int)
        SYSCALL2 (OS_TCSETPGRP, sys_tcsetpgrp, int, pid_t)
        SYSCALL2 (OS_KILL, sys_kill, pid_t, int)
        SYSCALL1 (OS_UNAME, sys_uname, struct utsname *)
        SYSCALL3 (OS_IOCTL, sys_ioctl, int, int, void *)
        SYSCALL1 (OS_MMAP, sys_mmap, struct os_mmap_request *)
        SYSCALL3 (OS_FCNTL, sys_fcntl, int, int, int)
        SYSCALL2 (OS_TCGETATTR, sys_tcgetattr, int, struct termios *)
        SYSCALL3 (OS_TCSETATTR, sys_tcsetattr, int, int, const struct termios *)
        SYSCALL4 (OS_PTHREAD_CREATE, sys_pthread_create, pthread_t * , const pthread_attr_t * , void *, void *)
        SYSCALL2R(OS_PTHREAD_JOIN, sys_pthread_join, pthread_t, void **)
        SYSCALL3 (OS_SIGACTION, sys_sigaction, int, const struct sigaction *, struct sigaction *)
        SYSCALL2 (OS_GETITIMER, sys_getitimer, int, struct itimerval *)
        SYSCALL3 (OS_SETITIMER, sys_setitimer, int, const struct itimerval *, struct itimerval *)
        SYSCALL5R(OS_SELECT, sys_select, int, fd_set *, fd_set *, fd_set *, struct timeval *)
        SYSCALL1 (OS_PIPE, sys_pipe, int *)
        SYSCALL1 (OS_ISATTY, sys_isatty, int)
        SYSCALL2 (OS_SYSLOG, sys_syslog, int, const char *)
        SYSCALL2 (OS_UTIME, sys_utime, const char *, const struct utimbuf *)
        SYSCALL2 (OS_RENAME, sys_rename, const char *, const char *)
        SYSCALL0R(OS_PAUSE, sys_pause)
        SYSCALL1R(OS_ALARM, sys_alarm, unsigned)
        SYSCALL2 (OS_GETMNTINFO, sys_getmntinfo, struct statvfs *, int)
        SYSCALL3 (OS_READLINK, sys_readlink, const char *, char *, size_t)
        SYSCALL2 (OS_SYMLINK, sys_symlink, const char *, const char *)
        SYSCALL2 (OS_LINK, sys_link, const char *, const char *)
        SYSCALL2 (OS_FTRUNCATE, sys_ftruncate, int, off_t)
        SYSCALL4 (OS_FSTATAT, sys_fstatat, int, const char *, struct stat *, int)
        SYSCALL1 (OS_FCHDIR, sys_fchdir, int)
        SYSCALL1 (OS_GETPGID, sys_getpgid, pid_t)
        SYSCALL3 (OS_SOCKET, sys_socket, int, int, int)
        SYSCALL3 (OS_BIND, sys_bind, int, const struct sockaddr *, socklen_t)
        SYSCALL3R(OS_ACCEPT, sys_accept, int, struct sockaddr *, socklen_t *)
        SYSCALL3 (OS_CONNECT, sys_connect, int, const struct sockaddr *, socklen_t)
#undef SYSCALL0
#undef SYSCALL0R
#undef SYSCALL1
#undef SYSCALL1R
#undef SYSCALL2
#undef SYSCALL2R
#undef SYSCALL3
#undef SYSCALL3R
#undef SYSCALL4
#undef SYSCALL5R
        default:
            kprintf("unknown syscall: %d\n", regs->eax);
            regs->eax = -ENOSYS;
        }

        if (syscall != OS_FORK)
            switch_task(regs);

    } else {
        kprintf("unexpected isr %d\n", regs->number);
        panic("unexpected isr");
    }
}

#define _(x) void isr##x(void);
#include "isr_numbers.h"
#undef _
#define _(x) void irq##x(void);
#include "irq_numbers.h"
#undef _
void isr128(void);

static void init_idt()
{
    /* remap pic irq table to 32-47, to deconflict with cpu triggered interrupts (0-31) */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20); /* start vector 32 */
    outb(0xA1, 0x28); /* start vector 40 */
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    memset(idt, 0, sizeof(idt));

#define _(x) idt_set(x, (uint32_t)isr##x, 0x08, 0x8E);
#include "isr_numbers.h"
#undef _
#define _(x) idt_set(x+32, (uint32_t)irq##x, 0x08, 0x8E);
#include "irq_numbers.h"
#undef _
    idt_set(128, (uint32_t)isr128, 0x08, 0x8E | 0x60); /* syscall */

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;

    idt_flush(&idt_ptr);
}

/* kernel alloc */

#include "heap.h"

static uint32_t placement_address;
static int use_halloc = 0;
static Halloc kheap = {0};

static uint32_t kmalloc_core(uint32_t size, int align, uint32_t * phys, const char * tag)
{
    if (use_halloc) {
        uint32_t addr = (uint32_t)halloc(&kheap, size, align ? 4096 : 0, tag);
        if (phys) {
            page_entry *page = get_page_entry(addr, 0, current_directory);
            *phys = page->frame*0x1000 + (addr & 0xFFF);
        }
        return addr;
    } else {
        if (align && (placement_address & 0xFFF)) {
            placement_address &= 0xFFFFF000;
            placement_address +=     0x1000;
        }

        if (phys)
            *phys = placement_address;

        uint32_t tmp = placement_address;
        placement_address += size;
        return tmp;
    }
}

void * kmalloc(uint32_t size, const char * tag)
{
    return (void *)kmalloc_core(size, 0, 0, tag);
}

static uint32_t kmalloc_a(uint32_t size, const char * tag)
{
    return kmalloc_core(size, 1, 0, tag);
}

#if 0
static uint32_t kmalloc_p(uint32_t size, uint32_t * phys)
{
    return kmalloc_core(size, 0, phys);
}
#endif

uint32_t kmalloc_ap(uint32_t size, uint32_t * phys, const char * tag)
{
    return kmalloc_core(size, 1, phys, tag);
}

void * krealloc(void * buf, unsigned int size)
{
    return hrealloc(&kheap, buf, size);
}

void kfree(void * ptr)
{
    if (use_halloc)
        hfree(&kheap, ptr);
}


/* paging */

page_directory *kernel_directory;
page_directory *current_directory;

static void dump_directory(const page_directory * dir, const char * name, int hexdump)
{
    kprintf("DUMP DIRECTORY %p '%s' 0x%x\n", dir, name, dir->tables_physical);
    for (unsigned int i = 0; i < 1024; i++) {
        if (dir->tables[i]) {
            kprintf(" tables[%d]= (virt 0x%x) &0x%x\n", i, i*1024*4096, dir->tables[i]);
            for (unsigned int j = 0; j < 1024; j++) {
                if (dir->tables[i]->pages[j].present) {
                    kprintf("  [0x%x] (virt 0x%x) frame=0x%x, present:%d, rw:%d, user:%d\n", j, (i*1024+j)*4096, dir->tables[i]->pages[j].frame, dir->tables[i]->pages[j].present, dir->tables[i]->pages[j].rw, dir->tables[i]->pages[j].user);
                    if (dir->tables[i]->pages[j].user && hexdump)
                        khexdump((void *)((i*1024+j)*4096), 0x1000);
                }
            }
        }
        if (dir->tables_physical[i])
            kprintf(" tables_physical[%d] = &0x%x\n", i, dir->tables_physical[i]);
    }
    kprintf(" physical_address: &0x%x\n", dir->physical_address);
    kprintf("\n");
}


/* bitset implementation */

static uint32_t *frames;
static uint32_t nframes;

#define INDEX_FROM_BIT(a)  ((a) / 32)
#define OFFSET_FROM_BIT(a) ((a) % 32)

static void modify_frame(uint32_t frame_address, int set)
{
    uint32_t frame = frame_address / 0x1000;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);

    if (set)
        frames[idx] |=   1 << off;
    else
        frames[idx] &= ~(1 << off);
}

static void set_frame(uint32_t frame_address)
{
    modify_frame(frame_address, 1);
}

static void clear_frame(uint32_t frame_address)
{
    modify_frame(frame_address, 0);
}

static unsigned int count_used_frames()
{
    unsigned int used = 0;
    for (unsigned int i = 0; i < INDEX_FROM_BIT(nframes); i++) {
        if (frames[i] != 0xFFFFFFFF) {
            for (unsigned int j = 0; j < 32; j++) {
                if (frames[i] & (1UL << j))
                    used++;
            }
        } else {
            used += 32;
        }
    }
    return used;
}

/* find the first empty slot in the bit frame */
static uint32_t first_frame()
{
    for (unsigned int i = 0; i < INDEX_FROM_BIT(nframes); i++) {
        if (frames[i] != 0xFFFFFFFF) {
            for (unsigned int j = 0; j < 32; j++) {
                if (!(frames[i] & (1UL << j)))
                    return i * 32 + j;
            }
        }
    }
    panic("memory exhausted");
    return 0;
}

static void alloc_frame(page_entry * page, int is_kernel, int is_writeable)
{
    if (page->present)
        panic("alloc_frame: page already present\n");

    page->present = 1;
    page->rw      = !!is_writeable;
    page->user    = !is_kernel;
    page->frame   = first_frame();
    set_frame(page->frame * 0x1000);
}

static void free_frame(page_entry *page)
{
    if (!page->present)
        panic("free_frame: page not present");

    clear_frame(page->frame * 0x1000);
    page->present = 0;
    page->frame   = 0xBEEF;
}

static void set_frame_identity(page_entry * page, int addr, int is_kernel, int is_writeable, int pat)
{
    page->present = 1;
    page->rw      = !!is_writeable;
    page->user    = !is_kernel;
    if (cpu_has_pat) {
        page->nocache = 0;
        page->writethrough = 0;
        page->pat = !!pat;
    }
    page->frame   = addr / 0x1000;
}

page_entry * get_page_entry(uint32_t address, int make, page_directory * dir)
{
    address /= 0x1000;
    uint32_t table_idx = address / 1024;

    if (dir->tables[table_idx])
        return &dir->tables[table_idx]->pages[address % 1024];

    if (!make)
        return 0;

    uint32_t tmp;
    dir->tables[table_idx] = (page_table *)kmalloc_ap(sizeof(page_table), &tmp, "pg-table");
    memset(dir->tables[table_idx], 0, sizeof(page_table));
    dir->tables_physical[table_idx] = tmp | 0x7; //FIXME: PRESENT, RW, US
    return &dir->tables[table_idx]->pages[address % 1024];
}

static void switch_page_directory(page_directory * dir)
{
    current_directory = dir;
    asm volatile("mov %0, %%cr3" : : "r" (dir->physical_address));
}

static void enable_paging(int enable)
{
    word cr0;
    asm volatile("mov %%cr0, %0" : "=r" (cr0));
    if (enable)
        cr0 |= 0x80000000;
    else
        cr0 &= ~0x80000000;
    asm volatile("mov %0, %%cr0" : : "r" (cr0));
}

void copy_page_physical(uint32_t src, uint32_t dst);

static page_table * clone_table(const page_table * src, uint32_t * physical_address)
{
    page_table * table = (page_table *)kmalloc_ap(sizeof(page_table), physical_address, "pg-table-clone");
    //kprintf("page_table %p (%d bytes)\n", table, sizeof(page_table));
    if (!table)
        return NULL;
    memset(table, 0, sizeof(page_table));

    // calculate memory required to clone page table
    int npages = 0;
    for (unsigned int i = 0; i < 1024; i++)
        if (src->pages[i].present)
            npages++;

    if (nframes - count_used_frames() < npages) {
        kfree(table);
        return NULL;
    }

    //copy each entry
    for (unsigned int i = 0; i < 1024; i++) {

        if (!src->pages[i].present)
            continue;

        alloc_frame(&table->pages[i], 0, 0); // user, not writable(what?)

#define _(x) if (src->pages[i].x) table->pages[i].x = 1;
_(present)
_(rw)
_(user)
_(accessed)
_(dirty)
#undef _

        //kprintf("copy page 0x%x -> 0x%x\n", src->pages[i].frame * 0x1000, table->pages[i].frame * 0x1000);

#if 1
        /* assembly is neccessary to prevent corruption (because tables->pages[i] may be a virtual address, and compiler doesn't know that */
        copy_page_physical(src->pages[i].frame * 0x1000, table->pages[i].frame * 0x1000);
#else
        enable_paging(0); // neccessary because we are building the new directory with these frames, and haven't updated tlb yet
        memcpy((void*)(table->pages[i].frame * 0x1000), (void *)(src->pages[i].frame * 0x1000), 0x1000);
        enable_paging(1);
#endif
    }

    return table;
}

static page_directory * clone_directory(const page_directory * src)
{
    uint32_t phys;

    page_directory * dir = (page_directory *)kmalloc_ap(sizeof(page_directory), &phys, "pg-dir-clone");
    if (!dir)
        return NULL;
    memset(dir, 0, sizeof(page_directory));

#if 0
    uint32_t offset = (uint32_t)dir->tables_physical - (uint32_t)dir;
    dir->physical_address = phys + offset;
#else
    dir->physical_address = phys + offsetof(page_directory, tables_physical);
#endif

    //copy each page
    for (unsigned int i = 0; i < 1024; i++) {
        if (!src->tables[i])
            continue;
        if (src->tables[i] == kernel_directory->tables[i]) {
            dir->tables[i] = src->tables[i];
            dir->tables_physical[i] = src->tables_physical[i];
        } else {
            dir->tables[i] = clone_table(src->tables[i], &phys);
            if (!dir->tables[i])
                return NULL; //FIXME: memory leak
            dir->tables_physical[i] = phys | 0x7;
        }
    }

    return dir;
}

/* remove all non-kernel pages from the directory */

static void clean_directory(page_directory * dir)
{
    for (unsigned int i = 0; i < 1024; i++) {
        if (!dir->tables[i] || dir->tables[i] == kernel_directory->tables[i])
            continue;
        for (unsigned int j = 0; j < 1024; j++) {
            if (dir->tables[i]->pages[j].present) {
                free_frame(&dir->tables[i]->pages[j]);
            }
        }

        kfree(dir->tables[i]);

        dir->tables[i] = NULL;
        dir->tables_physical[i] = 0;
    }

    switch_page_directory(current_directory);
}

static int alloc_frames(unsigned int start, int size, page_directory * directory, int is_kernel, int is_writable)
{
      if (nframes - count_used_frames() < (size + 0xFFF) / 0x1000)
          return -ENOMEM;

      for (unsigned int i = (unsigned int)start; i < (unsigned int)start + size; i += 0x1000)
          alloc_frame(get_page_entry(i, 1, directory), is_kernel, is_writable);

      return 0;
}

static int grow_cb(Halloc * cntx, unsigned int extra)
{
    if (alloc_frames((unsigned int)cntx->end, extra, cntx->directory, cntx->is_kernel, cntx->is_writable) < 0)
        return -ENOMEM;

    switch_page_directory(current_directory);
    cntx->end = (uint8_t *)cntx->end + extra;
    return 0; /* always succeeds */
}

static void shrink_cb(Halloc * cntx, unsigned int boundary_addr)
{
    for (unsigned int i = boundary_addr; i < (unsigned int)cntx->end; i += 0x1000) {
        free_frame(get_page_entry(i, 0, cntx->directory));
    }
    switch_page_directory(current_directory);
}

static void dump_cb(Halloc * cntx)
{
    dump_directory(cntx->directory, "directory", 0);
}

static void kheap_panic()
{
    panic("kheap");
}

static void init_paging(uint32_t low_size, uint32_t up_start, uint32_t up_size)
{
    uint32_t mem_end_page = up_start + up_size;

    placement_address = up_start;

    /* allocate bitset for each 'page' (which is called a frame here) */
    nframes = mem_end_page / 0x1000;
    nframes &= ~31; // INDEX_FROM_BIT works on 32 pages at a time only
    frames = kmalloc(INDEX_FROM_BIT(nframes) * sizeof(*frames), "nframes");
    memset(frames, 0, INDEX_FROM_BIT(nframes));

    // kernel page directory
    kernel_directory = (page_directory *)kmalloc_a(sizeof(page_directory), "pg-dir-kernel");
    memset(kernel_directory, 0, sizeof(page_directory));
    kernel_directory->physical_address = (uint32_t)kernel_directory->tables_physical;

    // mark all the frames [0x0) and [0x80000 ... 0x100000) as in use
    set_frame(0);
    for (unsigned int i = low_size; i < 0x100000; i += 0x1000)
        set_frame(i);

    // identity map the kernel and initial heap, mark these frames as in use
    {
        unsigned int i = (uint32_t)&multiboot;
        while (i < placement_address + 0x1000) {  //FIXME: need extra placement address space for initial heap page
            set_frame(i);
            set_frame_identity(get_page_entry(i, 1, kernel_directory), i, 1, 0, 0);
            i += 0x1000;
        }
    }

    switch_page_directory(kernel_directory);
    enable_paging(1);

    /* create kernel heap */

    int initial_size = 4096;
    for (unsigned int i = 0xC0000000; i < 0xC0000000 + initial_size; i += 0x1000)
        alloc_frame(get_page_entry(i, 1, kernel_directory), 1, 0);

    //dump_directory(kernel_directory, "kernel");

    halloc_init(&kheap, (void *)0xC0000000, initial_size);
    kheap.directory   = kernel_directory;
    kheap.is_kernel   = 1;
    kheap.is_writable = 0;
    kheap.grow_cb   = grow_cb;
    kheap.shrink_cb = shrink_cb;
    kheap.dump_cb = dump_cb;
    kheap.printf = kprintf;
    kheap.abort = kheap_panic;

    use_halloc = 1;

    /* pre-allocate additional *page tables* so kernel heap can grow up to max_size without
       encountering this recursive allocation problem:
                kmalloc() -> grow_cb() -> get_page_entry() -> kmalloc() -> ...
       the biggest user of kmalloc() currently is ext2_read() for large files */

    int max_size = 64 * 1024 * 1024;
    for (unsigned int i = 0xC0000000; i < 0xC0000000 + max_size; i += 0x1000)
        get_page_entry(i, 1, kernel_directory);

    /* identity map framebuffer or textmode buffer */

    if (fb_get_base()) {
        for (unsigned int i = fb_get_base(); i < fb_get_end(); i += 0x1000)
            set_frame_identity(get_page_entry(i, 1, kernel_directory), i, 0, 1, 1);
    } else {
        unsigned int i = 0xb8000;
        set_frame_identity(get_page_entry(i, 1, kernel_directory), i, 1, 0, 1);
    }

    /* identity map bios region */

    for (unsigned int i = 0xe0000; i < 0x100000; i += 0x1000)
        set_frame_identity(get_page_entry(i, 1, kernel_directory), i, 1, 0, 1);

    /* clone kernel directory, and switch to that */

    current_directory = clone_directory(kernel_directory);
    switch_page_directory(current_directory);
}


/* timer */

static void init_timer(int freq)
{
#if 1
    uint32_t divisor = 1193180 / freq;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
#else
    /* as slow as possible */
    outb(0x43, 0x36);
    outb(0x40, 0xFF);
    outb(0x40, 0xFF);
#endif
}

/* tasking */

#define KERNEL_STACK_SIZE 4096
#define USER_STACK_SIZE 0x30000 /* 192k */

#define USER_STACK_TOP 0xDFFFFFFC

static void move_stack(void * new_stack_start, unsigned int size, int spray)
{
    // allocate new stack pages...
    for (unsigned int i = (uint32_t)new_stack_start;
         i > (uint32_t)new_stack_start - size;
         i -= 0x1000) {
        alloc_frame(get_page_entry(i, 1, current_directory), 0 /*user mode */, 1 /* writable */ );
    }

    // now that page table has changed, need to flush tlb cache
    switch_page_directory(current_directory);

#if SPRAY_MEMORY
    for (unsigned int i = (uint32_t)new_stack_start;
         i > (uint32_t)new_stack_start - size;
         i -= 0x1000) {
        memset((void*)(i & ~0xfff), spray, 0x1000);
    }
#endif
}

static void init_sigact(Task * t)
{
    memset(t->proc->act, 0, sizeof(t->proc->act));
}

static void init_itimer(struct itimerval * it)
{
    memset(it, 0, sizeof(struct itimerval));
}

static struct timespec timespec_zero()
{
    return (struct timespec){0, 0};
}

#include <unistd.h>
#include <fcntl.h>

Task * idle_task;

static void init_tasking(const char * console_dev)
{
    current_task = ready_queue   = kmalloc(sizeof(Task) + sizeof(Process), "task-init");
    strlcpy(current_task->name, "init task", sizeof(current_task->name));
    current_task->id             = next_pid++;
    current_task->ppid           = 0;
    current_task->state          = STATE_RUNNING;
    current_task->next           = 0;
    current_task->thread_parent = NULL;
    current_task->stack_top = USER_STACK_TOP;
    move_stack((void*)USER_STACK_TOP, USER_STACK_SIZE, 0xf0);

    /* process specific */
    current_task->proc = (Process *)(current_task + 1);
    current_task->proc->pgrp  = current_task->id;
    current_task->proc->page_directory = current_directory;
    current_task->proc->stack_next = USER_STACK_TOP - USER_STACK_SIZE;
    current_task->proc->joins = NULL;

    strlcpy(current_task->proc->cwd, "/", sizeof(current_task->proc->cwd));

    memset(current_task->proc->fd, 0, sizeof(current_task->proc->fd));

    current_task->proc->fd[STDIN_FILENO] = vfs_open(console_dev, O_RDONLY, 0);
    current_task->proc->fd[STDOUT_FILENO] = vfs_open(console_dev, O_WRONLY, 0);
    current_task->proc->fd[STDERR_FILENO] = vfs_open(console_dev, O_WRONLY, 0);
    if (!current_task->proc->fd[STDIN_FILENO] || !current_task->proc->fd[STDOUT_FILENO] || !current_task->proc->fd[STDERR_FILENO]) {
        panic("vfs_open console failed");
    }
    sigemptyset(&current_task->proc->signal);
    init_sigact(current_task);
    init_itimer(&current_task->proc->timer);
    current_task->proc->alarm_armed = 0;
    current_task->proc->alarm = timespec_zero();
}

static int timespec_is_nonzero(const struct timespec a)
{
    return a.tv_sec || a.tv_nsec;
}

static int timespec_is_lte(const struct timespec a, const struct timespec b)
{
    if (a.tv_sec <= b.tv_sec)
        return 1;
    if (a.tv_sec == b.tv_sec && a.tv_nsec <= b.tv_nsec)
        return 1;
    return 0;
}

#if 0
static void dump_fd_set(const fd_set * fds)
{
    for (int i = 0; i < FD_SETSIZE; i++)
        if (FD_ISSET(i, fds))
            kprintf(" %d", i);
}
#endif

static int process_fd_sets(Task * t, int nfds, const fd_set * readfds, const fd_set * writefds, fd_set * r_result, fd_set * w_result)
{
    int count = 0;
    FD_ZERO(r_result);
    FD_ZERO(w_result);

    if (readfds)
        for (int i = 0; i < MIN(nfds, OPEN_MAX); i++)
            if (FD_ISSET(i, readfds) && t->proc->fd[i]) {
                if (vfs_read_available(t->proc->fd[i])) {
                    FD_SET(i, r_result);
                    count++;
                }
            }

    if (writefds)
        for (int i = 0; i < MIN(nfds, OPEN_MAX); i++)
            if (FD_ISSET(i, writefds) && t->proc->fd[i]) {
                if (vfs_write_available(t->proc->fd[i])) {
                    FD_SET(i, w_result);
                    count++;
                }
            }

    return count;
}

static Task * find_stopped(int ppid)
{
    for (Task * t = wait_queue; t; t = t->next)
        if (t->state == STATE_STOPPED && !t->u.stopped.parent_notified && t->ppid == ppid)
            return t;
    return NULL;
}

static Task ** find_zombie_pp(int ppid)
{
    Task ** pp;
    for (pp = &zombie_queue; *pp; pp = &(*pp)->next)
       if ((*pp)->ppid == ppid)
           return pp;
    return NULL;
}

#define MK_WAITPID_STATUS(errorlevel, stopped) ((((errorlevel) & 0xFF) << 8) | (stopped) << 16)

static int socket2(Task * t, FileDescriptor * other);

static int task_ready(Task * t)
{
    KASSERT(t->state != STATE_RUNNING);

    if (t->state == STATE_WAITPID) {
        Task * stopped = find_stopped(t->id);
        if (stopped) {
            if (t->u.waitpid.status) {
                switch_page_directory(t->proc->page_directory);
                *t->u.waitpid.status = MK_WAITPID_STATUS(0, 1);
            }
            stopped->u.stopped.parent_notified = 1;
            t->reg.eax = stopped->id;
            return 1;
        }
        Task ** pp = find_zombie_pp(t->id);
        if (pp) {
            Task * zombie = *pp;
            if (t->u.waitpid.status) {
                switch_page_directory(t->proc->page_directory);
                *t->u.waitpid.status = MK_WAITPID_STATUS(zombie->proc->errorlevel, 0);
            }
            t->reg.eax = zombie->id;
            *pp = zombie->next;
            kfree(zombie);
            return 1;
        }
    }

    if (t->state == STATE_NANOSLEEP && timespec_is_lte(t->u.nanosleep.expire, tnow)) {
        return 1;
    }

    if (t->state == STATE_READ) { // try and read some more

        int sz = t->u.read.size - t->u.read.pos;

        switch_page_directory(t->proc->page_directory);
        int ret = vfs_read(t->u.read.fd, t->u.read.buf + t->u.read.pos, sz);
        if (ret > 0)
            t->u.read.pos += ret;

        if (ret == -EAGAIN || (ret > 0 && (ret < sz && !vfs_isatty(t->u.read.fd)))) {
            /* go again */
        } else if (ret < 0) {
            t->reg.eax = ret;
            return 1;
        } else {
            t->reg.eax = t->u.read.pos; //size;
            return 1;
        }
    }

    if (t->state == STATE_WRITE) { // try and write some more

        int sz = t->u.write.size - t->u.write.pos;

        switch_page_directory(t->proc->page_directory);
        int ret = vfs_write(t->u.write.fd, t->u.write.buf + t->u.write.pos, sz);
        if (ret > 0)
            t->u.write.pos += ret;

        if (ret == -EAGAIN || (ret > 0 && ret < sz)) {
            /* need to go again */
        } else if (ret < 0) {
            t->reg.eax = ret;
            return 1;
        } else {
            t->reg.eax = t->u.write.pos;
            return 1;
        }
    }

    if (t->state == STATE_PTHREAD_JOIN) {
        for (Join * j = t->proc->joins; j; j = j->next) {
            if (j->thread == t->u.pthread_join.thread) {
                if (t->u.pthread_join.value_ptr) {
                    switch_page_directory(t->proc->page_directory);
                    *t->u.pthread_join.value_ptr = j->value;
                }
                return 1;
            }
        }
    }

    if (t->state == STATE_SELECT) {
        fd_set r_response, w_response;

        switch_page_directory(t->proc->page_directory);

        int count = process_fd_sets(t, t->u.select.nfds, t->u.select.readfds, t->u.select.writefds, &r_response, &w_response);

        if (r_response.fds_bits[0] || w_response.fds_bits[0] || (timespec_is_nonzero(t->u.select.expire) && timespec_is_lte(t->u.select.expire, tnow))) {
            if (t->u.select.readfds)
                *t->u.select.readfds = r_response;
            if (t->u.select.writefds)
                *t->u.select.writefds = w_response;
            if (t->u.select.errorfds)
                FD_ZERO(t->u.select.errorfds);
            t->reg.eax = count;
#if 0
            kprintf("    ^ select returned %d\n", count);
#endif
            return 1;
        }
    }

    if (t->state == STATE_ACCEPT) {
        if (vfs_is_connected(t->u.accept.fd)) {
            switch_page_directory(t->proc->page_directory);
            t->reg.eax = socket2(t, vfs_socket_get_other(t->u.accept.fd));
            return 1;
        }
    }

    return 0;
}

static void drain_wait_queue()
{
again:
    for (Task ** pt = &wait_queue; *pt; pt = &(*pt)->next) {
        Task * t = *pt;
        if (task_ready(t)) {
            *pt = t->next; //remove from wait queue

            t->next = ready_queue; //add to ready queue
            ready_queue = t;

            t->state = STATE_RUNNING;
            goto again;
        }
    }
}

static Task ** find_pp(Task *t);

static int do_signal_action(int i)
{
    kprintf("do_signal_action %d\n", i);

    if (i == SIGSTOP || i == SIGTSTP || i == SIGTTIN || i == SIGTTOU) {
        current_task->state = STATE_STOPPED;
        current_task->u.stopped.parent_notified = 0;
        sigdelset(&current_task->proc->signal, i);

        // move task to wait queue
        Task ** pp = find_pp(current_task);
        *pp = current_task->next;

        current_task->next = wait_queue;
        wait_queue = current_task;

        current_task = NULL;
        return 1;
    }

    if (i == SIGCONT && current_task->state == STATE_STOPPED) {
        current_task->state = STATE_RUNNING;
        sigdelset(&current_task->proc->signal, i);

        //move task to ready queue
        Task ** pp = find_pp(current_task);
        *pp = current_task->next;

        current_task->next = ready_queue;
        ready_queue = current_task;

        current_task = NULL;
        return 1;
    }

    return 0;
}

/* return 1 if task was killed */
static int process_signal(Task * t)
{
    current_task = t;
    switch_page_directory(t->proc->page_directory);
    int i;
    for (i = 0; i < 64 && !sigismember(&current_task->proc->signal, i); i++) ;
    if (i == 64)
        return 0; /* should never happen */
    kprintf("process_signal: GOT SIGNAL %d: %s\n", i, strsignal(i));
    sighandler_t handler = current_task->proc->act[i].sa_handler;
    if (handler != SIG_DFL && handler != SIG_IGN && i != SIGKILL && i != SIGSTOP) {
        //  create thread with handler
        pthread_t thread;
        kprintf("invoking handler\n");
        sys_pthread_create(&thread, NULL, (void *)(intptr_t)handler, (void *)i);
        current_task->proc->thread_signal = i;
        sigdelset(&current_task->proc->signal, i);
        current_task = NULL;
        return 0;
    }

    int ret = do_signal_action(i);
    if (ret)
        return ret;

    kprintf("terminating process\n");
    sys_exit(1);
    return 1;
}

static void check_for_signals()
{
again1:
    for (Task * t = ready_queue; t; t = t->next)
        if (!t->thread_parent && t->proc->signal)
            if (process_signal(t))
                goto again1;
again2:
    for (Task * t = wait_queue; t; t = t->next)
        if (!t->thread_parent && t->proc->signal)
            if (process_signal(t))
                goto again2;
}

///called *by* timer
static void switch_task(registers * reg)
{
    Task * task0 = (Task *)current_task;

#if 0
    kprintf("\nswitch: current task: Task:%x, pid=%d, eip=0x%x\n", current_task, current_task->id, reg->eip);
#endif

    drain_wait_queue();

    if (current_task)
        current_task->reg = *reg;

    check_for_signals();

    if (!current_task || current_task == idle_task) {
        current_task = ready_queue;
    } else {
        current_task = current_task->next;
        if (!current_task)
            current_task = ready_queue;
    }

    if (!current_task)
        current_task = idle_task;

    if (current_task == task0) {
        if (current_directory != current_task->proc->page_directory) /* happens if we handled state or responded to signal */
            switch_page_directory(current_task->proc->page_directory);
        return;
    }

#if 0
    kprintf("    switch: Task=%x(%d) -> Task=%x(%d), eip=0x%x, esp=0x%x\n", task0, task0 ? task0->id : -1, current_task, current_task->id, current_task->reg.eip, current_task->reg.esp);
#endif

    halloc_integrity_check(&kheap);
    *reg = current_task->reg;
    switch_page_directory(current_task->proc->page_directory);
}

void deliver_signal(int pgrp, int signal)
{
    kprintf("deliver signal %d to pgrp=%d\n", signal, pgrp);
    for (Task * t = ready_queue; t; t = t->next)
        if (t->proc->pgrp == pgrp)
            sigaddset(&t->proc->signal, signal);

    for (Task * t = wait_queue; t; t = t->next)
        if (t->proc->pgrp == pgrp)
            sigaddset(&t->proc->signal, signal);
}

static void deliver_signal_pid(int pid, int signal)
{
    kprintf("deliver signal %d to pid=%d\n", signal, pid);
    for (Task * t = ready_queue; t; t = t->next)
        if (t->id == pid)
            sigaddset(&t->proc->signal, signal);

    for (Task * t = wait_queue; t; t = t->next)
        if (t->id == pid)
            sigaddset(&t->proc->signal, signal);
}

static void deliver_signal_pgrp(int pgrp, int signal)
{
    kprintf("deliver signal %d to pgrp=%d\n", signal, pgrp);
    for (Task * t = ready_queue; t; t = t->next)
        if (t->proc->pgrp == pgrp)
            sigaddset(&t->proc->signal, signal);
    for (Task * t = wait_queue; t; t = t->next)
        if (t->proc->pgrp == pgrp)
            sigaddset(&t->proc->signal, signal);
}

static int sys_fork(const registers * reg)
{
    kprintf("sys_fork %p pid=%d\n", current_task, current_task ? current_task->id : - 1);

    page_directory * directory = clone_directory(current_directory);
    if (!directory)
        return -ENOMEM;

    Task * new_task = kmalloc(sizeof(Task) + sizeof(Process), "task-fork");
    memset(new_task, 0, sizeof(Task));
    strlcpy(new_task->name, current_task->name, sizeof(new_task->name));
    new_task->id = next_pid++;
    new_task->ppid = current_task->id;
    new_task->state = STATE_RUNNING;
    new_task->reg = *reg;
    new_task->reg.eax = 0;
    new_task->thread_parent = NULL;

    new_task->proc = (Process *)(new_task + 1);
    new_task->proc->pgrp = current_task->proc->pgrp;
    new_task->proc->page_directory = directory;
    strlcpy(new_task->proc->cwd, current_task->proc->cwd, sizeof(new_task->proc->cwd));
    vfs_dup_fds(new_task->proc->fd, current_task->proc->fd, OPEN_MAX);
    new_task->proc->joins = NULL;
    memcpy(new_task->proc->act, current_task->proc->act, sizeof(new_task->proc->act));
    sigemptyset(&new_task->proc->signal);
    init_itimer(&new_task->proc->timer);
    new_task->proc->alarm_armed = 0;
    new_task->proc->alarm = timespec_zero();

    new_task->proc->brk = current_task->proc->brk;

    /* the new task will get cpu time at switch_task() call */

#if 0
    new_task->next = 0;

    Task * tmp = (Task *)ready_queue;
    while (tmp->next)
        tmp = tmp->next;
    tmp->next = new_task;
#else
    new_task->next = (Task *)ready_queue;
    ready_queue = new_task;
#endif
    return new_task->id;
}

static int sys_getpid()
{
    return current_task->id;
}

static int sys_getppid()
{
    return current_task->ppid;
}

static void exit_thread(Task ** queue, Task * task)
{
    Task ** pp;
    for (pp = queue; *pp && *pp != task; pp = &(*pp)->next) ;
    *pp = task->next;

    kfree(task);
}

static void kill_threads(Task * task)
{
again1:
    for (Task * p = wait_queue; p; p = p->next)
        if (p->thread_parent == task) {
            exit_thread(&wait_queue, p);
            goto again1;
        }
again2:
    for (Task * p = ready_queue; p; p = p->next)
        if (p->thread_parent == task) {
            exit_thread(&ready_queue, p);
            goto again2;
        }
}

static Task ** find_pp(Task *t)
{
    Task ** pp;
    for (pp = &ready_queue; *pp; pp = &(*pp)->next)
       if (*pp == t)
           return pp;
    for (pp = &wait_queue; *pp; pp = &(*pp)->next)
       if (*pp == t)
           return pp;
    return NULL;
}

static void free_stack(uint32_t stack_top, uint32_t size)
{
    for (unsigned int i = stack_top; i > stack_top - size; i -= 0x1000)
        free_frame(get_page_entry(i, 0, current_directory));

    switch_page_directory(current_directory);
}

static int exit2(int status, int thread_termination)
{
    kprintf("sys_exit pid=%d, status=%d\n", current_task->id, status);

    Task * ct = current_task;

    /* remove current_task from ready/wait queue */
    {
        Task ** pp = find_pp(current_task);
        *pp = current_task->next;
    }

    free_stack(ct->stack_top, USER_STACK_SIZE);

    if (!current_task->thread_parent) {

        /* free child zombies */
        Task ** pp;
        while ((pp = find_zombie_pp(current_task->id))) {
            Task * zombie = *pp;
            *pp = zombie->next;
            kfree(zombie);
        }

        /* free file handles */
        for (unsigned int i = 0; i < OPEN_MAX; i++)
            if (ct->proc->fd[i])
                vfs_close(ct->proc->fd[i]);

        /* switch to kernel directory, as we are about to destroy current page directory */
        switch_page_directory(kernel_directory);

        /* free page directory and process frames */
        clean_directory(ct->proc->page_directory);
        kfree(ct->proc->page_directory);

        /* kill any threads belonging to this process */
        kill_threads(current_task);

        ct->proc->errorlevel = status;

        ct->state = STATE_ZOMBIE;
        ct->next = zombie_queue;
        zombie_queue = ct;

    } else { // is a thread
        if (!thread_termination)
            deliver_signal_pid(current_task->thread_parent->id, SIGKILL);
        kfree(ct);
    }

    current_task = NULL; /* rely on switch_task() after syscall */
    return 0;
}

static int sys_exit(int status)
{
    return exit2(status, 0);
}

static void move_current_task_to_wait_queue(const registers * reg)
{
    // remove from ready queue
    for (Task ** pt = &ready_queue; *pt; pt = &(*pt)->next)
        if (*pt == current_task) {
            *pt = current_task->next;
            current_task->next = NULL;
            break;
        }

    // add to wait queue
    current_task->next = wait_queue;
    wait_queue = current_task;

    current_task->reg = *reg;

    current_task = NULL;
}

static int sys_write(registers * reg, int fd, const void * buf, size_t size)
{
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;

    int ret = vfs_write(current_task->proc->fd[fd], buf, size);

    if (!(current_task->proc->fd[fd]->flags & O_NONBLOCK) && (ret == -EAGAIN || (ret > 0 && ret < size))) {
        current_task->state = STATE_WRITE;
        current_task->u.write.fd = current_task->proc->fd[fd];
        current_task->u.write.buf = buf;
        current_task->u.write.size = size;
        current_task->u.write.pos = ret > 0 ? ret : 0;
        move_current_task_to_wait_queue(reg);
        return 0;
    }

    return ret;
}

static int sys_read(registers * reg, int fd, void * buf, size_t size)
{
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;

    int ret = vfs_read(current_task->proc->fd[fd], buf, size);

    if (!(current_task->proc->fd[fd]->flags & O_NONBLOCK) && (ret == -EAGAIN || (ret > 0 && ret < size && !vfs_isatty(current_task->proc->fd[fd]) ))) {
        current_task->state = STATE_READ;
        current_task->u.read.fd = current_task->proc->fd[fd];
        current_task->u.read.buf = buf;
        current_task->u.read.size = size;
        current_task->u.read.pos = ret > 0 ? ret : 0;
        move_current_task_to_wait_queue(reg);
        return 0;
    }

    return ret;
}

/* returns OPEN_MAX if there are no more free descriptors */
static int alloc_fd2(Task * t, int start)
{
    int fd;
    for (fd = start; fd < OPEN_MAX && t->proc->fd[fd]; fd++) ;
    return fd;
}

static int alloc_fd(Task * t)
{
    return alloc_fd2(t, STDERR_FILENO);
}

static int sys_open(const char * path, int flags, mode_t mode)
{
    int fd = alloc_fd(current_task);
    if (fd >= OPEN_MAX)
        return -EMFILE;
    current_task->proc->fd[fd] = vfs_open(path, flags, mode);
    kprintf("sys_open '%s' flags=0x%x, mode=0x%x, result=%p, fd=%d\n", path, flags, mode, current_task->proc->fd[fd], fd);
    if (!current_task->proc->fd[fd])
        return -ENOENT;
    return fd;
}

static int sys_close(int fd)
{
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;

    vfs_close(current_task->proc->fd[fd]);
    current_task->proc->fd[fd] = NULL;
    return 0;
}

static off_t sys_lseek(int fd, off_t offset, int whence)
{
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;

    return vfs_lseek(current_task->proc->fd[fd], offset, whence);
}

static int sys_getdents(int fd, struct dirent * de, size_t count)
{
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;

    return vfs_getdents(current_task->proc->fd[fd], de, count);
}

static int sys_dup(int fd)
{
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;
    int fd2 = alloc_fd(current_task);
    if (fd2 >= OPEN_MAX)
        return -EMFILE;
    vfs_dup_fds(&current_task->proc->fd[fd2], &current_task->proc->fd[fd], 1);
    return fd2;
}

static int sys_dup2(int fd, int fd2)
{
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;
    if (fd2 < 0 || fd2 >= OPEN_MAX)
        return -EBADF;
    if (current_task->proc->fd[fd2])
        vfs_close(current_task->proc->fd[fd2]);
    vfs_dup_fds(&current_task->proc->fd[fd2], &current_task->proc->fd[fd], 1);
    return fd2;
}

static int sys_unlink(const char *path)
{
    return vfs_unlink(path, 0);
}

static int sys_rmdir(const char *path)
{
    return vfs_unlink(path, 1);
}

static int sys_mkdir(const char *path, mode_t mode)
{
    return vfs_mkdir(path, mode);
}

static int sys_brk(uint32_t addr, uint32_t * current_brk)
{
    if (addr) {
        if (addr > current_task->proc->brk) {
            if (alloc_frames(current_task->proc->brk, addr - current_task->proc->brk, current_directory, 0, 1) < 0) /* user, write */
                return -ENOMEM;
            current_task->proc->brk = addr;
        } else {
            addr += 0xfff;
            addr &= ~0xfff;
            for (unsigned int i = addr; i < current_task->proc->brk; i += 0x1000) {
                free_frame(get_page_entry(i, 0, current_directory));
            }
            current_task->proc->brk = addr;
        }
        switch_page_directory(current_directory);
    }

    if (current_brk)
        *current_brk = current_task->proc->brk;
    return 0;
}

static void kthread_exit(int value)
{
    kprintf("kthread exit: %x\n", value);
    while(1);
}

static Task * create_task(void (*eip)(int ), unsigned int stack_size)
{
    uint8_t *  stack = (void *) kmalloc_a(stack_size, "kernel-task-stack");
    uint32_t * stack_values = (uint32_t *)(stack + stack_size);

    stack_values[-1] =  0; /* eip */
    stack_values[-2] =  0; /* ebp */
    stack_values[-3] =  0xbeef; /* param */
    stack_values[-4] =  (uint32_t)kthread_exit;

    Task * new_task = kmalloc(sizeof(Task) + sizeof(Process), "task-kernel");
    strlcpy(new_task->name, "kernel-task", sizeof(new_task->name));
    new_task->id = next_pid++;
    new_task->ppid = 0;
    new_task->state = STATE_RUNNING;
#if SPRAY_MEMORY
    memset(&new_task->reg, 0xFA, sizeof(new_task->reg));
#endif
    new_task->reg.esp = (uint32_t)(stack_values - 4);
    new_task->reg.eip = (uint32_t)eip;
    new_task->reg.cs = 0x8;
    new_task->reg.ds = 0x10;
    new_task->reg.ss = 0x10;
    new_task->reg.eax = 0;
    new_task->reg.eflags = 0x200;
    new_task->thread_parent = NULL;

    new_task->proc = (Process *)(new_task + 1);
    new_task->proc->pgrp = new_task->id;
    new_task->proc->page_directory = kernel_directory;
    strlcpy(new_task->proc->cwd, "/", sizeof(new_task->proc->cwd));
    memset(new_task->proc->fd, 0, sizeof(new_task->proc->fd)); // HAS NO FDS
    new_task->proc->joins = NULL;
    sigemptyset(&new_task->proc->signal);
    init_sigact(new_task);
    init_itimer(&new_task->proc->timer);
    new_task->proc->alarm_armed = 0;
    new_task->proc->alarm = timespec_zero();

    new_task->next = NULL;

    return new_task;
}

#include "elf.h"

static void create_vm_block(uint32_t addr, uint32_t size)
{
    unsigned int i;
    for (i = addr & ~0xfff; i < addr + size; i += 0x1000)
        alloc_frame(get_page_entry(i, 1, current_directory), 0, 1); /* user, write */

    switch_page_directory(current_directory);

#if SPRAY_MEMORY
    for (i = addr & ~0xfff; i < addr + size; i += 0x1000) {
        memset((void *)i, 0xf4, 0x1000);
    }
#endif

    current_task->proc->brk = i;
}

//FIXME: no error checking

static uint32_t create_task_elf_fd(FileDescriptor * fd)
{
    ElfHeader e;
    vfs_read(fd, &e, sizeof(ElfHeader));

    if (e.e_ident[0] != 0x7F || e.e_ident[1] != 'E' || e.e_ident[2] != 'L' || e.e_ident[3] != 'F') {
        kprintf("ELF header missing\n");
        return 0;
    }

    for (unsigned int i = 0; i < e.e_phnum; i++) {

        vfs_lseek(fd, e.e_phoff + i * sizeof(ElfPHeader), SEEK_SET);

        ElfPHeader p;
        vfs_read(fd, &p, sizeof(ElfPHeader));

        if (p.p_type != PT_LOAD)
            continue;

        create_vm_block(p.p_vaddr, p.p_memsz);

        vfs_lseek(fd, p.p_offset, SEEK_SET);

        vfs_read(fd, (void *)p.p_vaddr, p.p_filesz);
        if (p.p_memsz > p.p_filesz)
            memset((void *)(p.p_vaddr + p.p_filesz), 0, p.p_memsz - p.p_filesz);
    }

    vfs_close(fd);

    return e.e_entry;
}

static uint32_t create_task_elf(const char * path)
{
    FileDescriptor * fd = vfs_open(path, O_RDONLY, 0);
    if (!fd) {
        kprintf("create_task_elf: could not open: %s\n", path);
        return 0;
    }

    return create_task_elf_fd(fd);
}

static int vector_count(char * const v[])
{
    int i = 0;
    for (i = 0; v[i]; i++) ;
    return i;
}

static int vector_flat_size(char * const v[])
{
    int count = 0;
    for (int i = 0; v[i]; i++)
        count += sizeof(char *) + strlen(v[i]) + 1;
#define PAD(x) ((x + 4) & ~3)
    return PAD(count + sizeof(char *));
}

static void vector_dup2(void * dst, char * const v[])
{
    int c = vector_count(v);

    char ** pptr = (char **)dst;
    char * cptr = (char*)dst + (c + 1) * sizeof(char *);

    for (int i = 0; i < c; i++) {
        pptr[i] = cptr;
        int sz = strlen(v[i]) + 1;
        memcpy(cptr, v[i], sz);
        cptr += sz;
    }
    pptr[c] = NULL;
}

static char ** vector_dup(char * const v[])
{
    void * buf = kmalloc(vector_flat_size(v), "vector-dup");
    if (!buf)
        panic("kmalloc failed");

    vector_dup2(buf, v);
    return buf;
}

static int sys_execve(registers * regs, const char * pathname, char * const argv[], char * const envp[])
{
    kprintf("sys_execve pid=%d pathname='%s'\n", current_task->id, pathname);
    for (int i = 0; argv[i]; i++)
        kprintf("\targv[%d]: '%s'\n", i, argv[i]);

    FileDescriptor * fd = vfs_open(pathname, O_RDONLY, 0);
    if (!fd)
        return -1;

    strlcpy(current_task->name, pathname, sizeof(current_task->name));

#if 0
    if (envp) {
        kprintf("envp=%p\n", envp);
        for (int i = 0; envp[i]; i++) {
            kprintf("envp[%d]: '%s'\n", i, envp[i]);
        }
    } else
        kprintf("env is null\n");
#endif

    /* clone argv */

    char ** argv_local = vector_dup(argv);
    char ** envp_local = vector_dup(envp);

#if 0
    for (int i = 0; argv_local[i]; i++) {
        kprintf("argv_local[%d]: '%s'\n", i, argv_local[i]);
    }
#endif

    kill_threads(current_task);

    clean_directory(current_task->proc->page_directory);

    current_task->stack_top = USER_STACK_TOP;
    move_stack((void*)USER_STACK_TOP, USER_STACK_SIZE, 0xf1);
    current_task->proc->stack_next = USER_STACK_TOP - USER_STACK_SIZE;

    regs->eip = create_task_elf_fd(fd);
    if (!regs->eip) {
        sys_exit(1);
        return -1;
    }

    vfs_close(fd);

    /* populate stack with argv and envp */
    int argv_size = vector_flat_size(argv_local);
    int envp_size = vector_flat_size(envp_local);

    regs->esp = USER_STACK_TOP - 8 - argv_size - envp_size;
    regs->ebp = 0;

    uint32_t * stack_values = (uint32_t *)(USER_STACK_TOP - argv_size - envp_size);
    stack_values[0] = USER_STACK_TOP - envp_size + 4; //envp
    stack_values[-1] = USER_STACK_TOP - argv_size - envp_size + 4; //argv
    stack_values[-2] = vector_count(argv_local); //argc

    void * envp_stack = (uint32_t*)(USER_STACK_TOP - envp_size + 4);
    vector_dup2(envp_stack, envp_local);

    void * argv_stack = (uint32_t*)(USER_STACK_TOP - argv_size - envp_size + 4);
    vector_dup2(argv_stack, argv_local);

    sigemptyset(&current_task->proc->signal);
    init_sigact(current_task);
    init_itimer(&current_task->proc->timer);
    current_task->proc->alarm_armed = 0;
    current_task->proc->alarm = timespec_zero();

    kfree(argv_local);
    kfree(envp_local);

    for (int i = 0; i < OPEN_MAX; i++)
        if (current_task->proc->fd[i] && (current_task->proc->fd[i]->fd_flags & FD_CLOEXEC)) {
            vfs_close(current_task->proc->fd[i]);
            current_task->proc->fd[i] = NULL;
        }

    return 0;
}

static int has_child_process(int ppid)
{
    for (Task * t = zombie_queue; t; t = t->next)
        if (t->ppid == ppid)
            return 1;
    for (Task * t = wait_queue; t; t = t->next)
        if (t->ppid == ppid)
            return 1;
    for (Task * t = ready_queue; t; t = t->next)
        if (t->ppid == ppid)
            return 1;
    return 0;
}

static pid_t sys_waitpid(registers * reg, pid_t pid, int * stat_loc, int options)
{
    kprintf("sys_waitpid: pid=%d, stat_loc=%p, options=%d\n", pid, stat_loc, options);

    if (!has_child_process(current_task->id))
        return -ECHILD;

    Task * stopped = find_stopped(current_task->id);
    if (stopped) {
        if (stat_loc)
            *stat_loc = MK_WAITPID_STATUS(0, 1);
        stopped->u.stopped.parent_notified = 1;
        return stopped->id;
    }
    Task ** pp = find_zombie_pp(current_task->id);
    if (pp) {
        Task * zombie = *pp;
        if (stat_loc)
            *stat_loc = MK_WAITPID_STATUS(zombie->proc->errorlevel, 0);
        pid_t ret = zombie->id;
        *pp = zombie->next;
        kfree(zombie);
        return ret;
    }

    if (options & WNOHANG)
        return 0;

    current_task->state = STATE_WAITPID;
    current_task->u.waitpid.status = stat_loc;
    move_current_task_to_wait_queue(reg);
    return 0;
}

static int sys_getcwd(char * buf, size_t size)
{
    strlcpy(buf, current_task->proc->cwd, size);
    return 0;
}

void get_absolute_path(char * abspath, int size, const char * path)
{
    if (path[0]=='/' || !current_task)
        strlcpy(abspath, path, size);
    else {
        strlcpy(abspath, current_task->proc->cwd, size);
        if (abspath[strlen(abspath) - 1] != '/') {
            int l = strlen(abspath);
            abspath[l] = '/';
            abspath[l + 1] = 0;
        }
        strlcpy(abspath + strlen(abspath), path, size - strlen(abspath));
    }
}

static int sys_chdir(const char * path)
{
    char abspath[PATH_MAX];
    get_absolute_path(abspath, sizeof(abspath), path);

    FileDescriptor * fd = vfs_open(abspath, O_DIRECTORY, 0);
    if (!fd)
        return -1;
    vfs_close(fd);
    strlcpy(current_task->proc->cwd, abspath, sizeof(current_task->proc->cwd));
    return 0;
}

static int sys_stat(const char * path, struct stat * st)
{
    kprintf("sys_stat %s %p\n", path, st);
    return vfs_stat(NULL, path, st, 1 /* resolve symlinks to file */);
}

static int sys_lstat(const char * path, struct stat * buf)
{
    kprintf("sys_lstat %s %p\n", path, buf);
    return vfs_stat(NULL, path, buf, 0);
}

static int sys_fstat(int fd, struct stat * st)
{
    kprintf("sys_fstat %d %p\n", fd, st);
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;

    return vfs_fstat(current_task->proc->fd[fd], st);
}

static int sys_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    switch (clock_id) {
    case CLOCK_MONOTONIC:
    case CLOCK_REALTIME:
        *tp = tnow;
        return 0;
    }
    return -EINVAL;
}

static struct timespec timeval_to_timespec(const struct timeval a)
{
    return (struct timespec){.tv_sec = a.tv_sec, .tv_nsec = a.tv_usec * 1000};
}

static struct timespec timespec_add(const struct timespec a, const struct timespec b)
{
    struct timespec v;

    v.tv_sec = a.tv_sec + b.tv_sec;
    v.tv_nsec = a.tv_nsec + b.tv_nsec;
    if (v.tv_nsec > 1000000000) {
       v.tv_sec++;
       v.tv_nsec -= 1000000000;
    }

    return v;
}

static int sys_nanosleep(registers * reg, const struct timespec * ts, struct timespec * rem)
{
    current_task->state = STATE_NANOSLEEP;
    current_task->u.nanosleep.expire = timespec_add(tnow, *ts);
    move_current_task_to_wait_queue(reg);
    return 0;
}

static pid_t sys_getpgid(pid_t pid)
{
    kprintf("sys_getpgid: %d\n", pid);

    if (!pid)
        pid = current_task->id;

    for (Task * t = ready_queue; t; t = t->next)
        if (t->id == pid)
            return t->proc->pgrp;
    for (Task * t = wait_queue; t; t = t->next)
        if (t->id == pid)
            return t->proc->pgrp;

    return -1;
}

static int sys_setpgid(pid_t pid, pid_t pgid)
{
    kprintf("sys_setpgid: %d, %d\n", pid, pgid);

    if (!pid)
        pid = current_task->id;

    for (Task * t = ready_queue; t; t = t->next)
        if (t->id == pid)
            t->proc->pgrp = pgid;
    for (Task * t = wait_queue; t; t = t->next)
        if (t->id == pid)
            t->proc->pgrp = pgid;
    return 0;
}

static pid_t sys_getpgrp(void)
{
    return current_task->proc->pgrp;
}

static pid_t sys_setpgrp(void)
{
    current_task->proc->pgrp = current_task->id;
    return current_task->proc->pgrp;
}

static pid_t sys_tcgetpgrp(int fildes)
{
    if (fildes < 0 || fildes >= OPEN_MAX || !current_task->proc->fd[fildes])
        return -EBADF;

    return vfs_tcgetpgrp(current_task->proc->fd[fildes]);
}

static int sys_tcsetpgrp(int fd, pid_t pgrp)
{
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;

    return vfs_tcsetpgrp(current_task->proc->fd[fd], pgrp);
}

static int sys_kill(pid_t pid, int sig)
{
    if (pid > 0)
        deliver_signal_pid(pid, sig);
    else if (pid < -1)
        deliver_signal_pgrp(-pid, sig);
    else if (pid == 0)
        deliver_signal_pgrp(current_task->proc->pgrp, sig);
    else
        kprintf("sys_kill: unsupported pid %d\n", pid);
    return 0;
}

static int sys_uname(struct utsname * name)
{
    strlcpy(name->sysname, "os", sizeof(name->sysname));
    strlcpy(name->nodename, "localhost", sizeof(name->nodename));
    strlcpy(name->release, "1", sizeof(name->release));
    strlcpy(name->version, "stock", sizeof(name->version));
    strlcpy(name->machine, "i686", sizeof(name->machine));
    return 0;
}

static int sys_ioctl(int fd, int request, void * data)
{
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;
    int ret = vfs_ioctl(current_task->proc->fd[fd], request, data);
    if (ret < 0)
        kprintf("sys_ioctl: fd=%d, request=%d, data=%p\n", fd, request, data);
    return ret;
}

static int sys_mmap(struct os_mmap_request * req)
{
    int fd = req->fildes;
    kprintf("sys_mmap fd=%d, addr=%p, len=0x%x\n", fd, req->addr, req->len);
    if (fd < 0)
        return -ENOSYS;
    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;
    return vfs_mmap(current_task->proc->fd[fd], req);
}

static int sys_fcntl(int fd, int cmd, int value)
{
    if (fd < 0 || fd >= OPEN_MAX)
        return -EBADF;

    if (!current_task->proc->fd[fd])
        return -EBADF;

    switch(cmd) {
    case F_GETFD:
        return current_task->proc->fd[fd]->fd_flags;
    case F_SETFD:
        current_task->proc->fd[fd]->fd_flags = value;
        return 0;
    case F_GETFL:
        return current_task->proc->fd[fd]->flags;
    case F_SETFL:
        current_task->proc->fd[fd]->flags = value;
        return 0;
    case F_DUPFD:
        {
        int fd2 = alloc_fd2(current_task, value);
        if (fd2 >= OPEN_MAX)
            return -EMFILE;
        vfs_dup_fds(&current_task->proc->fd[fd2], &current_task->proc->fd[fd], 1);
        return fd2;
        }
    case F_SETLK:
        return 0;
    default:
        kprintf("sys_fcntl: fd=%d, cmd=%d, value=%d\n", fd, cmd, value);
    }

    return -EINVAL;
}

static int sys_tcgetattr(int fildes, struct termios * termios_p)
{
    kprintf("sys_tcgetattr\n");
    if (fildes < 0 || fildes >= OPEN_MAX || !current_task->proc->fd[fildes])
        return -EBADF;
    return vfs_tcgetattr(current_task->proc->fd[fildes], termios_p);
}

static int sys_tcsetattr(int fildes, int optional_actions, const struct termios * termios_p)
{
    kprintf("sys_tcsetattr\n");
    if (fildes < 0 || fildes >= OPEN_MAX || !current_task->proc->fd[fildes])
        return -EBADF;
    return vfs_tcsetattr(current_task->proc->fd[fildes], optional_actions, termios_p);
}

static int sys_pthread_create(pthread_t * thread, const pthread_attr_t * attr, void * start_routine, void * arg)
{
    kprintf("sys_pthread_create: %p, %p, %p, %p\n", thread, attr, start_routine, arg);

    Task * new_task = kmalloc(sizeof(Task), "task-thread");
    memset(new_task, 0, sizeof(Task));
    snprintf(new_task->name, sizeof(new_task->name), "%s:thread", current_task->name);
    *thread = new_task->id = next_pid++;
    new_task->ppid = current_task->id;
    new_task->state = STATE_RUNNING;
    new_task->reg = current_task->reg;
    new_task->reg.eip = (uint32_t)start_routine;
    new_task->reg.esp = current_task->proc->stack_next - 4;
    new_task->stack_top = current_task->proc->stack_next;
    move_stack((void*)(current_task->proc->stack_next), USER_STACK_SIZE, 0xf2);
    current_task->proc->stack_next -= USER_STACK_SIZE;

    new_task->reg.ebp = 0;
    new_task->thread_parent = current_task;

    new_task->proc = current_task->proc;
    current_task->proc->thread_signal = 0; /* when thread exits, don't do_signal_action */

    void ** stack_values = (void **)new_task->reg.esp;
    stack_values[1] = arg;
    stack_values[0] = (void *)(uint32_t)sys_pthread_join; /* thread termination indicator, will deliberatey trigger page fault */

    // insert into ready queue
    new_task->next = (Task *)ready_queue;
    ready_queue = new_task;
    return 0;
}

static int sys_pthread_join(registers * reg, pthread_t thread, void ** value_ptr)
{
    current_task->state = STATE_PTHREAD_JOIN;
    current_task->u.pthread_join.thread = thread;
    current_task->u.pthread_join.value_ptr = value_ptr;
    move_current_task_to_wait_queue(reg);
    return 0;
}

static int sys_sigaction(int sig, const struct sigaction * act, struct sigaction * oact)
{
    kprintf("sys_sigaction %d\n", sig);
    if (sig <= 0 || sig >= NSIG)
       return -EINVAL;
    if (oact)
        *oact = current_task->proc->act[sig];
    if (act) {
        current_task->proc->act[sig] = *act;
        if (act->sa_flags & SA_SIGINFO) {
            kprintf("warning: SA_SIGINFO not implemented\n");
        }
    }
    return 0;
}

static int sys_getitimer(int which, struct itimerval * value)
{
    if (which != ITIMER_REAL)
        return -EINVAL;
    *value = current_task->proc->timer;
    return 0;
}

static int sys_setitimer(int which, const struct itimerval * value, struct itimerval * ovalue)
{
    if (which != ITIMER_REAL)
        return -EINVAL;
    if (ovalue)
        *ovalue = current_task->proc->timer;
    current_task->proc->timer = *value;
    return 0;
}

static int sys_select(registers * reg, int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds, struct timeval * timeout)
{
#if 0
    kprintf("sys_select: nfds=%d, readfds={", nfds);
    if (readfds) dump_fd_set(readfds);
    kprintf(" }, writefds={");
    if (writefds) dump_fd_set(writefds);
    kprintf(" }, errorfds={");
    if (errorfds) dump_fd_set(errorfds);
    kprintf(" }, timeout=%d,%d\n", timeout ? timeout->tv_sec : -1, timeout ? timeout->tv_usec : -1);
#endif

    if (timeout && !timeout->tv_sec && !timeout->tv_usec) {
        fd_set r_result, w_result;
        process_fd_sets(current_task, nfds, readfds, writefds, &r_result, &w_result);
        if (readfds)
            *readfds = r_result;
        if (writefds)
            *writefds = w_result;
        if (errorfds)
            FD_ZERO(errorfds);
        return 0;
    }

    current_task->state = STATE_SELECT;
    current_task->u.select.nfds = nfds;
    current_task->u.select.readfds = readfds;
    current_task->u.select.writefds = writefds;
    current_task->u.select.errorfds = errorfds;

    if (timeout)
        current_task->u.select.expire = timespec_add(tnow, timeval_to_timespec(*timeout));
    else
        current_task->u.select.expire = timespec_zero(); /* wait indefinitely */

    move_current_task_to_wait_queue(reg);
    return 0;
}

static int sys_pipe(int * fildes)
{
    fildes[0] = alloc_fd(current_task);
    if (fildes[0] >= OPEN_MAX)
        return -EMFILE;

    //we need two fds. so reserve this fd, so next call to alloc_fd() skips over it
    current_task->proc->fd[fildes[0]] = (FileDescriptor *)0x1;

    fildes[1] = alloc_fd(current_task);
    if (fildes[1] >= OPEN_MAX)
        return -EMFILE;

    kprintf("sys_pipe allocated readfd %d, writefd %d\n", fildes[0], fildes[1]);

    return vfs_pipe(&current_task->proc->fd[fildes[0]], &current_task->proc->fd[fildes[1]]);
}

static int sys_isatty(int fildes)
{
    if (fildes < 0 || fildes >= OPEN_MAX || !current_task->proc->fd[fildes])
        return -EBADF;

    return vfs_isatty(current_task->proc->fd[fildes]);
}

static int sys_syslog(int priority, const char * message)
{
    kprintf("syslog[%d]: %d: %s\n", current_task->id, priority, message);
    return 0;
}

static int sys_utime(const char * path, const struct utimbuf *times)
{
    return vfs_utime(path, times);
}

static int sys_rename(const char * old, const char * new)
{
    kprintf("sys_rename: '%s' -> '%s'\n", old, new);
    return vfs_rename(old, new);
}

static int sys_pause(registers * reg)
{
    kprintf("sys_pause pid=%d\n", current_task->id);
    current_task->state = STATE_PAUSE;
    move_current_task_to_wait_queue(reg);
    return 0;
}

static unsigned sys_alarm(registers * reg, unsigned seconds)
{
    unsigned ret = current_task->proc->alarm.tv_sec;
    current_task->proc->alarm_armed = !!seconds;
    if (seconds) {
        current_task->proc->alarm.tv_sec = seconds;
        current_task->proc->alarm.tv_nsec = 0;
    } else {
        current_task->proc->alarm = timespec_zero(); /* disable */
    }
    return ret;
}

static int sys_getmntinfo(struct statvfs * mntbufp, int size)
{
    return vfs_getmntinfo(mntbufp, size);
}

ssize_t sys_readlink(const char * path, char * buf, size_t bufsize)
{
    return vfs_readlink(path, buf, bufsize);
}

int sys_symlink(const char * path1, const char * path2)
{
    return vfs_link(path1, path2, 1);
}

int sys_link(const char * path1, const char * path2)
{
    return vfs_link(path1, path2, 0);
}

int sys_ftruncate(int fildes, off_t length)
{
    if (fildes < 0 || fildes >= OPEN_MAX || !current_task->proc->fd[fildes])
        return -EBADF;

    return vfs_truncate(current_task->proc->fd[fildes], length);
}

int sys_fstatat(int fd, const char * path, struct stat * buf, int flag)
{
    if (fd == AT_FDCWD)
        return vfs_stat(NULL, path, buf, !(flag & AT_SYMLINK_NOFOLLOW));

    if (fd < 0 || fd >= OPEN_MAX || !current_task->proc->fd[fd])
        return -EBADF;

    return vfs_stat(current_task->proc->fd[fd], path, buf, !(flag & AT_SYMLINK_NOFOLLOW));
}

int sys_fchdir(int fildes)
{
    if (fildes < 0 || fildes >= OPEN_MAX || !current_task->proc->fd[fildes])
        return -EBADF;

    return sys_chdir(current_task->proc->fd[fildes]->path);
}

static int socket2(Task * t, FileDescriptor * other)
{
    int fd = alloc_fd(t);
    if (fd >= OPEN_MAX)
        return -EMFILE;
    t->proc->fd[fd] = vfs_socket(other);
    return t->proc->fd[fd] ? fd : -ENOMEM;
}

int sys_socket(int domain, int type, int protocol)
{
    kprintf("socket[%d]: domain=%d, type=%d, protocol=%d\n", current_task->id, domain, type, protocol);
    if (domain != AF_UNIX || type != SOCK_STREAM)
        return -EINVAL;
    return socket2(current_task, NULL);
}

int sys_bind(int socket, const struct sockaddr * address, socklen_t address_len)
{
    if (socket < 0 || socket >= OPEN_MAX || !current_task->proc->fd[socket])
        return -EBADF;
    if (address->sa_family != AF_UNIX)
        return -EINVAL;
    const struct sockaddr_un * sun = (const struct sockaddr_un *)address;
    kprintf("bind[%d] socket=%d, path='%s'\n", current_task->id, socket, sun->sun_path);
    return vfs_bind(current_task->proc->fd[socket], sun->sun_path);
}

int sys_accept(registers * reg, int socket, struct sockaddr * address, socklen_t * address_len)
{
    kprintf("accept[%d] socket=%d\n", current_task->id, socket);
    if (socket < 0 || socket >= OPEN_MAX || !current_task->proc->fd[socket])
        return -EBADF;
    int ret = vfs_accept(current_task->proc->fd[socket]);
    if (ret < 0)
        return ret;
    current_task->state = STATE_ACCEPT;
    current_task->u.accept.fd = current_task->proc->fd[socket];
    //FIXME: return address, address_len
    move_current_task_to_wait_queue(reg);
    return 0;
}

int sys_connect(int socket, const struct sockaddr * address, socklen_t address_len)
{
    if (socket < 0 || socket >= OPEN_MAX || !current_task->proc->fd[socket])
        return -EBADF;
    if (address->sa_family != AF_UNIX)
        return -EINVAL;
    const struct sockaddr_un * sun = (const struct sockaddr_un *)address;
    kprintf("connect[%d] socket=%d, path='%s'\n", current_task->id, socket, sun->sun_path);
    return vfs_connect(current_task->proc->fd[socket], sun->sun_path);
}

static void dump_task(Task * t, int dump_fds)
{
    kprintf("\tpid=%5d, ppid=%5d, pgrp=%5d, state=%s, [%s], chdir=%s, ip=%p\n", t->id, t->ppid, t->proc->pgrp, state_names[t->state], t->name, t->proc->cwd, t->reg.eip);
#if 0
    if (t->id != 2) {
        switch_page_directory(t->proc->page_directory);
        dump_registers(&t->reg);
        switch_page_directory(current_directory);
    }
#endif
    if (dump_fds) {
        for (int i = 0; i < OPEN_MAX; i++) {
            if (t->proc->fd[i])
                kprintf("\t\tfd=%d, %s\n", i, t->proc->fd[i]->path);
        }
    }
}

static void dump_process_queue(Task * head, int dump_fds, const char * name)
{
    kprintf("%s\n", name);
    for (Task * t = head; t; t = t->next)
        dump_task(t, dump_fds);
    kprintf("\n");
}

void dump_processes()
{
    kprintf("---\n");
    dump_process_queue(ready_queue, 1, "ready_queue");
    dump_process_queue(wait_queue, 1, "wait_queue");
    dump_process_queue(zombie_queue, 0, "zombie_queue");
    kprintf("current_task\n");
    dump_task(current_task, 1);
    kprintf("\nmemory\n\t%d/%d frames (%d/%d KiB) in use\n\n", count_used_frames(), nframes, count_used_frames()*4, nframes*4);
    vfs_dump_sockets();
#if 0
    kprintf("\nkernel heap\n");
    halloc_dump(&kheap);
#endif
}

/* proc */

static void bprintf(char ** buf, ssize_t * size, const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char * new_buf = krealloc(*buf, *size + n + 1);
    if (!new_buf)
        return;

    va_start(args, fmt);
    vsnprintf(new_buf + *size, n + 1, fmt, args);
    va_end(args);

    *buf = new_buf;
    *size = *size + n;
}

#define printf(...) bprintf(&fd->buf, &fd->buf_size, __VA_ARGS__)
static void proc_meminfo(FileDescriptor * fd)
{
    printf("%d/%d KiB in use\n", count_used_frames()*4, nframes*4);
}

static void print_process_queue(FileDescriptor * fd, Task * head, const char * name)
{
    printf("%s:\n", name);
    for (Task * t = head; t; t = t->next)
        printf("pid=%3d, ppid=%3d, pgrp=%3d, state=%s, name=%s, chdir=%s\n", t->id, t->ppid, t->proc->pgrp, state_names[t->state], t->name, t->proc->cwd);
    printf("\n");
}

static void proc_psinfo(FileDescriptor * fd)
{
    print_process_queue(fd, ready_queue, "ready_queue");
    print_process_queue(fd, wait_queue, "wait_queue");
    print_process_queue(fd, zombie_queue, "zombie_queue");
}
#undef printf

typedef struct {
    uint8_t boot;
    uint8_t starting_head;
    uint16_t starting_cylinder;
    uint8_t system_id;
    uint8_t ending_head;
    uint16_t ending_cylinder;
    uint32_t relative_sector;
    uint32_t total_sectors;
} MBR;

static unsigned int get_first_partition_offset(const char * path)
{
    int offset = 0;

    FileDescriptor * fd = vfs_open(path, O_RDONLY, 0);
    if (!fd)
        return 0;

    uint8_t sector[512];
    if (vfs_read(fd, sector, sizeof(sector)) != sizeof(sector))
        goto done;

    if (sector[510] != 0x55 || sector[511] != 0xAA)
        goto done;

    const MBR * r = (const MBR *)(sector + 0x1BE);
    if (r->boot != 0x80)
        goto done;

    offset = r->relative_sector * 512;

done:
    vfs_close(fd);
    return offset;
}

/*
 *
 * kernel main
 *
 */

#if 0
static int pci_enum(void * cntx, int bus, int slot, int func)
{
    kprintf("PCI %02x:%02x.%x, vendor=0x%04x, device=0x%04x, class=0x%x\n", bus, slot, func,
        pci_read(bus, slot, func, PCI_VENDOR_ID, 2),
        pci_read(bus, slot, func, PCI_DEVICE_ID, 2),
        pci_read(bus, slot, func, PCI_CLASS, 1));
    return 0;
}
#endif

void jmp_to_userspace(uint32_t eip, uint32_t esp);

static void idle(int param);

void start2(uint32_t magic, const void * info, uint32_t initial_esp);
void start2(uint32_t magic, const void * info, uint32_t initial_esp)
{
    uint32_t low_size, up_start, up_size, mod_start, mod_end;
    kprintf("Hello world, magic=0x%x, info=%p, esp=0x%x\n", magic, info, initial_esp);
    if (magic == 0x2BADB002)
        parse_multiboot_info(info, &low_size, &up_start, &up_size, &mod_start, &mod_end);
    else if (magic == 0x1337)
        parse_linux_params(info, &low_size, &up_start, &up_size, &mod_start, &mod_end);
    else
        panic("invalid magic number");

    cpu_init();

    init_gdt();

    init_idt();

    init_timer(TICKS_PER_SECOND);

    init_paging(low_size, up_start, up_size);

#if 0
    pci_scan(pci_enum, NULL);
#endif

    char root_dev[128];
    get_cmdline_token("root=", "/dev/module", root_dev, sizeof(root_dev));

    dev_init();
    vfs_register_mount_point2(2, "dev", &dev_io, NULL, DEV_INODE_MIN);

    dev_register_device("null", &null_dio, 0, NULL, NULL);
    dev_register_device("urandom", &urandom_dio, 0, NULL, NULL);
    dev_register_device("power", &power_dio, 0, NULL, NULL);

    if (mod_end - mod_start > 0) {
        void * module = mem_init((void *)mod_start, mod_end - mod_start);
        dev_register_device("module", &mem_dio, 0, mem_getsize, module);
    }

    void * ata = ata_init();
    if (ata) {
        dev_register_device("hda", &ata_dio, 0, ata_getsize, ata);
        unsigned int offset = get_first_partition_offset("/dev/hda");
        kprintf("first partition offset: %d\n", offset);
        if (offset) {
            void * loop = loop_init("/dev/hda", offset);
            dev_register_device("hda1", &loop_dio, 0, NULL, loop);
        }
    }

    void * bios = mem_init((void *)0, 0x100000);
    dev_register_device("mem", &mem_dio, 0, mem_getsize, bios);

    if (fb_get_base())
        dev_register_device("fb0", &fb_io, 0, NULL, NULL);

    dev_register_device("console0", &tty_dio, 1, NULL, NULL);

    serial_init();
    dev_register_device("serial0", &serial_dio, 1, NULL, NULL);

    void * ne2k = ne2k_init();
    if (ne2k)
        dev_register_device("net", &ne2k_dio, 1, NULL, ne2k);

    proc_init();
    proc_register_file("meminfo", proc_meminfo);
    proc_register_file("psinfo", proc_psinfo);
    vfs_register_mount_point2(2, "proc", &proc_io, NULL, PROC_INODE_MIN);

    void * ext2 = ext2_init(root_dev);
    if (!ext2)
       panic("ext2_init failed");
    vfs_register_mount_point2(1, "", &ext2_io, ext2, 2);

    char tty_dev[128];
    get_cmdline_token("console=", "/dev/console0", tty_dev, sizeof(tty_dev));
    dev_register_symlink("tty", tty_dev);

    init_tasking("/dev/tty"); /* must come after paging */

    idle_task = create_task(idle, 0x1000);

    uint32_t entry = create_task_elf("/bin/init");
    if (!entry)
        panic("create task elf failed");

    uint32_t g_kernel_stack = kmalloc_a(KERNEL_STACK_SIZE, "kernel-stack");
    set_kernel_stack(g_kernel_stack + KERNEL_STACK_SIZE);

    rtc_init();

    asm volatile("sti");

    kprintf("\n\nswitching to user mode:\n");

    uint32_t * stack_values = (uint32_t *)USER_STACK_TOP;
    stack_values[0] = 0; //envp
    stack_values[-1] = 0; //argv
    stack_values[-2] = 0; //argc
    jmp_to_userspace((uint32_t)entry, USER_STACK_TOP - 8);

    /* never reach here */
}

static void idle(__attribute((unused)) int param)
{
    for (;;) asm("hlt");
}
