/* based on https://wiki.osdev.org/Serial_Ports */
#include <string.h>
#include "utils.h"
#include "ringbuffer.h"
#include <termios.h>
#include "serial.h"
#include <errno.h>
#include <sys/ioctl.h>

#define PORT 0x3F8

static int serial_received()
{
    return inb(PORT + 5) & 1;
}

static int is_transmit_empty()
{
    return inb(PORT + 5) & 0x20;
}

static void putc(int ch)
{
    while (is_transmit_empty() == 0);
    outb(PORT, ch);
}

#if 0
#include <stdarg.h>
#include "generic_printf.h"
static void serputc(void *cntx, int c)
{
    tty->putc(c);
}
static int serprintf(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    generic_vprintf(serputc, NULL, fmt, args);
    va_end(args);
    return 0; //FIXME: return bytes printed
}
#endif

int serial_foreground_pgrp = 0;
int serial_oflag = OPOST | ONLCR;
int serial_lflag = ECHO | ISIG;
static char rb_buffer[512];
static RingBuffer rb;

static void serial_irq(void * opaque)
{
    while (serial_received() == 0);
    int ch = inb(PORT);
    if (ch == '\r')
        ch = '\n';
    if (ch == 127)
        ch = '\b';
    if (serial_lflag & ECHO)
        putc(ch);
    ringbuffer_write1(&rb, ch);
}

static void serial_putc(int ch)
{
    if (ch == '\n')
        putc('\r');
    putc(ch);
}

void serial_init()
{
    ringbuffer_init(&rb, rb_buffer, sizeof(rb_buffer));

    outb(PORT + 1, 0x00);
    outb(PORT + 3, 0x80);
    outb(PORT + 0, 0x03);
    outb(PORT + 1, 0x00);
    outb(PORT + 3, 0x03);
    outb(PORT + 2, 0xC7);
    outb(PORT + 4, 0x0B);
    outb(PORT + 4, 0x1E);
    outb(PORT + 0, 0xAE);
    if (inb(PORT + 0) != 0xAE) {
        kprintf("serial_init: error\n");
        return;
    }
    outb(PORT + 4, 0x0F);

    outb(PORT + 1, 0x01);
    irq_handler[4] = serial_irq;

    serial_putc('\r');
    serial_putc('\n');
}

static int serial_write(FileDescriptor * fd, const void * buf, int size)
{
    for (int i = 0; i < size; i++)
        serial_putc(((uint8_t *)buf)[i]);

    return size;
}

static int serial_read(FileDescriptor * fd, void * buf, int size)
{
    uint8_t * buf8 = buf;
    int i;
    if (!ringbuffer_read_available(&rb))
        return -EAGAIN;
    for (i = 0; i < size && ringbuffer_read_available(&rb); i++) {
        buf8[i] = ringbuffer_read1(&rb);
        if (buf8[i] == 0x4) /* end of xmit */
            return MAX(0, i - 1);
    }
    return i;
}

static int serial_read_available(const FileDescriptor * fd)
{
    return ringbuffer_read_available(&rb);
}

static int serial_tcgetpgrp(FileDescriptor * fd)
{
    return serial_foreground_pgrp;
}

static int serial_tcsetpgrp(FileDescriptor * fd, pid_t pgrp)
{
    serial_foreground_pgrp = pgrp;
    return 0;
}

static int serial_tcgetattr(FileDescriptor * fd, struct termios * termios_p)
{
    memset(termios_p, 0, sizeof(struct termios));
    termios_p->c_lflag = serial_lflag;
    termios_p->c_oflag = serial_oflag;
    return 0;
}

static int serial_tcsetattr(FileDescriptor * fd, int optional_actions, const struct termios * termios_p)
{
    serial_lflag = termios_p->c_lflag;
    serial_oflag = termios_p->c_oflag;
    kprintf("tcsetattr: c_iflag=0x%x c_oflag=0x%x\n", termios_p->c_iflag, termios_p->c_oflag);
    return 0;
}

static int serial_ioctl(FileDescriptor * fd, int request, void * data)
{
    if (request == TIOCGWINSZ) {
        struct winsize * ws = data;
        ws->ws_row = 25;
        ws->ws_col = 80;
        return 0;
    }
    return -EINVAL;
}

const DeviceOperations serial_dio = {
    .write = serial_write,
    .read = serial_read,
    .read_available = serial_read_available,
    .ioctl = serial_ioctl,
    .tcgetpgrp = serial_tcgetpgrp,
    .tcsetpgrp = serial_tcsetpgrp,
    .tcgetattr = serial_tcgetattr,
    .tcsetattr = serial_tcsetattr,
};
