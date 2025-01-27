#include "tty.h"
#include "ringbuffer.h"
#include "utils.h"
#include <errno.h>
#include <signal.h>

int kb_rawmode = 0;

static const char charmap[][2] = {
    [0x01] = { 27, 27},

    [0x02] = {'1', '!'},
    [0x03] = {'2', '@'},
    [0x04] = {'3', '#'},
    [0x05] = {'4', '$'},
    [0x06] = {'5', '%'},
    [0x07] = {'6', '^'},
    [0x08] = {'7', '&'},
    [0x09] = {'8', '*'},
    [0x0a] = {'9', '('},
    [0x0b] = {'0', ')'},
    [0x0c] = {'-', '_'},
    [0x0d] = {'=', '+'},
    [0x0e] = {'\b', '\b' },

    [0x0f] = {'\t','\t' },
    [0x10] = {'q', 'Q' },
    [0x11] = {'w', 'W' },
    [0x12] = {'e', 'E' },
    [0x13] = {'r', 'R' },
    [0x14] = {'t', 'T' },
    [0x15] = {'y', 'Y' },
    [0x16] = {'u', 'U' },
    [0x17] = {'i', 'I' },
    [0x18] = {'o', 'O' },
    [0x19] = {'p', 'P' },
    [0x1a] = {'[', '{' },
    [0x1b] = {']', '}' },

    [0x1c] = {'\n', '\n' },

    [0x1e] = {'a', 'A' },
    [0x1f] = {'s', 'S' },
    [0x20] = {'d', 'D' },
    [0x21] = {'f', 'F' },
    [0x22] = {'g', 'G' },
    [0x23] = {'h', 'H' },
    [0x24] = {'j', 'J' },
    [0x25] = {'k', 'K' },
    [0x26] = {'l', 'L' },
    [0x27] = {';', ':' },
    [0x28] = {'\'', '"' },

    [0x29] = {'`', '~' },

    [0x2b] = {'\\','|'},

    [0x2c] = {'z', 'Z'},
    [0x2d] = {'x', 'X'},
    [0x2e] = {'c', 'C'},
    [0x2f] = {'v', 'V'},
    [0x30] = {'b', 'B'},
    [0x31] = {'n', 'N'},
    [0x32] = {'m', 'M'},
    [0x33] = {',', '<'},
    [0x34] = {'.', '>'},
    [0x35] = {'/', '?'},

    [0x37] = {'*', '*'},

    [0x39] = {' ', ' '},

    [0x4a] = {'-', '-'},
    [0x4e] = {'+', '+'},
};

static char rb_buffer[8];
static RingBuffer rb;

static void process_scancode(unsigned int scancode);

#define PS2_DATA 0x60
#define PS2_CMD 0x64
#define PS2_STATUS 0x64

static void kb_irq(void * opaque)
{
    int scancode = inb(PS2_DATA);
    process_scancode(scancode);
}

static char mouse_rb_buffer[32];
static RingBuffer mouse_rb;

static int mouse_state = 0;
static char mouse_buffer[3];

static void mouse_irq(void * opaque)
{
    int byte = inb(PS2_DATA);
    switch(mouse_state) {
    case 0:
        mouse_buffer[0] = byte;
        if (!(byte & 0x08))
            break;
        mouse_state++;
        break;
    case 1:
        mouse_buffer[1] = byte;
        mouse_state++;
        break;
    case 2:
        mouse_buffer[2] = byte;
        mouse_state = 0;
        if (ringbuffer_write_available(&mouse_rb) >= 3)
            ringbuffer_write(&mouse_rb, mouse_buffer, 3);
    }
}

static int ps2_wait_write()
{
    volatile unsigned int timer = 10000;
    while (inb(PS2_STATUS) & 2 && --timer > 0) ;
    return timer > 0;
}

static int ps2_wait_read()
{
    volatile unsigned int timer = 10000;
    while (!(inb(PS2_STATUS) & 1) && --timer > 0) ;
    return timer > 0;
}

static void write_command(int command)
{
    if (!ps2_wait_write()) {
        kprintf("ps2: write_command timeout\n");
        return;
    }
    outb(PS2_CMD, command);
}

static void write_data(int data)
{
    if (!ps2_wait_write()) {
        kprintf("ps2: write_data timeout\n");
        return;
    }
    outb(PS2_DATA, data);
}

static int read_data()
{
    if (!ps2_wait_read())
        return -1;
    return inb(PS2_DATA);
}

static void write_data_device(int device, int data)
{
    if (device)
        write_command(0xd4); //write second
    write_data(data);
}

void kb_init()
{
    write_data(0xff); //reset
    read_data();
    read_data();

    write_command(0xad); //disable first port
    write_command(0xa7); //disable second port

    read_data(); //drain buffer

    write_command(0x20); //read config
    int config = read_data();
    config |= (1<<0) | (1<<1); //first port interrupt, second port interrupt
    write_command(0x60);
    write_data(config);

    write_command(0xae); //enable first port
    ringbuffer_init(&rb, rb_buffer, sizeof(rb_buffer));
    irq_handler[1] = kb_irq;

    write_command(0xa8); //enable second port
    write_data_device(1, 0xf6); //set defaults
    write_data_device(1, 0xf4); //enable scan codes

    ringbuffer_init(&mouse_rb, mouse_rb_buffer, sizeof(mouse_rb_buffer));
    irq_handler[12] = mouse_irq;
}

int kb_available()
{
    return ringbuffer_read_available(&rb);
}

int kb_read(char * buf, int size, int process_end_of_xmit)
{
    int i;
    if (!kb_available())
        return -EAGAIN;
    for (i = 0; i < size && kb_available(); i++) {
        buf[i] = ringbuffer_read1(&rb);
        if (process_end_of_xmit && buf[i] == 0x4) /* end of xmit */
            return MAX(0, i - 1);
    }
    return i;
}

void kb_put(int c)
{
    if (tty_lflag & ECHO)
        tty->putc(c);
    ringbuffer_write1(&rb, c);
}

void kb_puts(char * s)
{
    while (*s) kb_put(*s++);
}

static int lcontrol = 0;
static int alt = 0;
static int lshift = 0;
static int rshift = 0;
static int capslock = 0;

enum {
    KEY_LEFT_CONTROL = 0x1d,
    KEY_LEFT_SHIFT = 0x2a,
    KEY_RIGHT_SHIFT = 0x36,
    KEY_CAPSLOCK = 0x3a,
    KEY_F1 = 0x3B,
    KEY_F2 = 0x3C,
    KEY_F3 = 0x3D,
    KEY_F4 = 0x3E,
    KEY_ALT = 0x38,
    KEY_PAUSE = 0x45,
    KEY_HOME = 0x47,
    KEY_UP = 0x48,
    KEY_PGUP = 0x49,
    KEY_LEFT = 0x4B,
    KEY_END = 0x4F,
    KEY_RIGHT = 0x4D,
    KEY_DOWN = 0x50,
    KEY_PGDN = 0x51,
    KEY_INS = 0x52,
    KEY_DEL = 0x53,
    KEY_LEFT_WINDOWS = 0x5b,
    KEY_RIGHT_WINDOWS = 0x5d
};

static int toupper(int c)
{
    return c >= 'a' && c <= 'z' ? c &= ~0x20 : c;
}

static void arrow(int c)
{
    kb_put('\033');
    if (tty_kbmode != 2)
        kb_puts(!tty_kbmode ? (lcontrol ? "[1;5" : (alt ? "[1;3" : "[")) : "O");
    kb_put(c);
}

static void process_scancode(unsigned int scancode)
{
    if (kb_rawmode) {
        if (scancode == KEY_PAUSE)
            dump_processes();
        kb_put(scancode);
        return;
    }

    if (scancode & 0x80) { /* up */
        scancode &= ~0x80;

        if (scancode == KEY_LEFT_CONTROL)
            lcontrol = 0;
        else if (scancode == KEY_ALT)
            alt = 0;
        else if (scancode == KEY_LEFT_SHIFT)
            lshift = 0;
        else if (scancode == KEY_RIGHT_SHIFT)
            rshift = 0;

    } else { /* down */
        int c;
        if (scancode == KEY_LEFT_CONTROL)
            lcontrol = 1;
        else if (scancode == KEY_ALT)
            alt = 1;
        else if (scancode == KEY_LEFT_SHIFT)
            lshift = 1;
        else if (scancode == KEY_RIGHT_SHIFT)
            rshift = 1;
        else if (scancode == KEY_CAPSLOCK)
            capslock ^= 1;
        else if (scancode < NB_ELEMS(charmap) && (c = charmap[scancode][lshift || rshift])) {
            if (capslock && ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
                c ^= 0x20;
            if (lcontrol) {
                c = toupper(c);
                if (tty_lflag & ISIG) {
                    if (c == 'C') {
                        deliver_signal_pgrp(tty_foreground_pgrp, SIGINT);
                        return;
                    } else if (c == 'Z') {
                        deliver_signal_pgrp(tty_foreground_pgrp, SIGTSTP);
                        return;
                    } else if (c == '\\' || c == '/') {
                        deliver_signal_pgrp(tty_foreground_pgrp, SIGQUIT);
                        return;
                    }
                }
                kb_put(c - '@');
                return;
            }
            kb_put(c);
        } else if (scancode == KEY_UP) {
            arrow('A');
        } else if (scancode == KEY_DOWN) {
            arrow('B');
        } else if (scancode == KEY_RIGHT) {
            arrow('C');
        } else if (scancode == KEY_LEFT) {
            arrow('D');
        } else if (scancode == KEY_INS) {
            kb_puts("\033[2~");
        } else if (scancode == KEY_DEL) {
            kb_puts("\033[3~");
        } else if (scancode == KEY_HOME) {
            kb_puts("\033[H");
        } else if (scancode == KEY_END) {
            kb_puts("\033[F");
        } else if (scancode == KEY_PGUP) {
            kb_puts("\033[5~");
        } else if (scancode == KEY_PGDN) {
            kb_puts("\033[6~");
        } else if (scancode == KEY_F1) {
            kb_puts("\033OP");
        } else if (scancode == KEY_F2) {
            kb_puts("\033OQ");
        } else if (scancode == KEY_F3) {
            kb_puts("\033OR");
        } else if (scancode == KEY_F4) {
            kb_puts("\033OS");
        } else if (scancode == KEY_PAUSE) {
            dump_processes();
        } else if (scancode == KEY_LEFT_WINDOWS || scancode == KEY_RIGHT_WINDOWS) {
            /* do nothing */
        } else
            kprintf("[scancode 0x%x]", scancode);
    }
}

static int mouse_read(FileDescriptor * fd, void * buf, int size)
{
    if (!ringbuffer_read_available(&mouse_rb))
        return -EAGAIN;
    return ringbuffer_read(&mouse_rb, buf, size);
}

static int mouse_read_available(const FileDescriptor * fd)
{
    return ringbuffer_read_available(&mouse_rb);
}

const DeviceOperations mouse_dio = {
    .read  = mouse_read,
    .read_available  = mouse_read_available,
};
