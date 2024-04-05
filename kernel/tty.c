#include "ringbuffer.h"
#include "tty.h"
#include "utils.h"
#include "vfs.h"
#include <ctype.h>
#include <errno.h>
#include <string.h>

const TTYCommands * tty = &textmode_commands;
int tty_foreground_pgrp = 0;
int tty_oflag = OPOST | ONLCR;
int tty_lflag = ECHO | ISIG | ICANON;
int tty_inverse = 0;
int tty_kbmode = 0;
int tty_autowrap = 1;
static int saved_inverse = 0;

void tty_puts(const char * s)
{
    while (*s)
        tty->putc(*s++);
}

/* emulate vt100 processor */

enum {
    STATE_NORMAL = 0,
    STATE_ESCAPE = 1,
    STATE_CODE = 2,
    STATE_HASH = 3,
    STATE_CHARSET_G0 = 4,
    STATE_CHARSET_G1 = 5
};
static int esc_mode = STATE_NORMAL;

#define MAX_NB_ARGS 8
static int args[MAX_NB_ARGS];
static int args_index;

static void rendition(int x)
{
     //kprintf("tty: rendition %d\n", x);
     if (x == 0)
         tty_inverse = 0;
     else if (x == 7)
         tty_inverse = 0xFF;
     else
         kprintf("tty: command m: %d\n", x);
}

static void tty_process(int c)
{
    if (esc_mode == STATE_ESCAPE) {
        if (c == 'c') { // reset
            tty_oflag = OPOST | ONLCR;
            tty_lflag = ECHO | ISIG | ICANON;
            tty_inverse = 0;
            tty_kbmode = 0;
            tty_autowrap = 1;
            tty->clear(TTY_PAGE);
            tty->set_pos(0, 0);
            tty->reset();
        } else if (c == '[') {
            esc_mode = STATE_CODE;
            memset(args, 0, sizeof(args));
            args_index = -1;
            return;
        } else if (c == '#') {
            esc_mode = STATE_HASH;
            return;
        } else if (c == '(') {
            esc_mode = STATE_CHARSET_G0;
            return;
        } else if (c == ')') {
            esc_mode = STATE_CHARSET_G1;
            return;
        } else if (c == '=') { /* enter alternate keypad mode */
        } else if (c == '>') { /* exit alternate keypad mode */
        } else if (c == '<') { /* enter ansi mode (exit vt52 mode) */
        } else if (c == 'D') {
            tty->move_cursor_scroll(TTY_DOWN_SCROLL);
        } else if (c == 'E') {
            tty->move_cursor_scroll(TTY_CRLF_SCROLL);
        } else if (c == 'M') {
            tty->move_cursor_scroll(TTY_UP_SCROLL);
        } else if (c == '7') {
            tty->save_cursor();
            saved_inverse = tty_inverse;
        } else if (c == '8') {
            tty->restore_cursor();
            tty_inverse = saved_inverse;
        } else {
            kprintf("tty: unsupported escape character: %c\n", c);
        }
        esc_mode = STATE_NORMAL;
        return;
    } else if (esc_mode == STATE_CODE) {
        if (isdigit(c)) {
            if (args_index == -1)
                args_index++;
            if (args_index < MAX_NB_ARGS)
                args[args_index] = args[args_index]*10 + c - '0';
            return;
        } else if (c == '?') {
            return;
        } else if (c == ';') {
            if (args_index != -1)
                args_index++;
            return;
        } else if (c == 'A') {
            tty->move_cursor(TTY_UP, args[0]);
        } else if (c == 'B') {
            tty->move_cursor(TTY_DOWN, args[0]);
        } else if (c == 'C') {
            tty->move_cursor(TTY_RIGHT, args[0]);
        } else if (c == 'D') {
            tty->move_cursor(TTY_LEFT, args[0]);
        } else if (c == 'f') {
            args_index++;
            if (args_index == 2)
                tty->set_pos(args[1] - 1, args[0] - 1);
            else
                kprintf("tty: f command; unexpected number of arguments\n");
        } else if (c == 'c') {
            kb_puts("\033[?1;0c");
        } else if (c == 'h') {
            if (args_index != -1)
                args_index++;
            if (args_index > 1)
                 kprintf("tty: multiple j commands\n");
            if (args[0] == 1)
                tty_kbmode = 1; /* ascii set */
            else if (args[0] == 7)
                tty_autowrap = 1;
            else
                kprintf("tty: command h: %d\n", args[0]);
        } else if (c == 'l') {
            if (args_index != -1)
                args_index++;
            if (args_index > 1)
                kprintf("tty: multiple l commands\n");
            if (args[0] == 1)
                tty_kbmode = 0; /* ascii reset */
            else if (args[0] == 2)
                tty_kbmode = 2; /* vt52 */
            else if (args[0] == 7)
                tty_autowrap = 0;
            else
                kprintf("tty: command l: %d\n", args[0]);
        } else if (c == 'm') { /* attributes */
            if (args_index == -1)
                rendition(0);
            else {
                args_index++;
                for (int i =0; i < args_index; i++)
                    rendition(args[i]);
            }
        } else if (c == 'r') {
            tty->scroll_region(args[0], args[1]);
        } else if (c == 'H') {
            tty->set_pos(args[1] - 1, args[0] - 1);
        } else if (c == 'J') {
            if (args[0] == 0)
                tty->clear(TTY_PAGE_AFTER);
            else if (args[0] == 1)
                tty->clear(TTY_PAGE_BEFORE);
            else if (args[0] == 2)
                tty->clear(TTY_PAGE);
            else {
                 kprintf("tty: unsupported J mode %d\n", args[0]);
            }
        } else if (c == 'K') {
            if (args[0] == 0)
                tty->clear(TTY_LINE_AFTER);
            else if (args[0] == 1)
                tty->clear(TTY_LINE_BEFORE);
            else if (args[0] == 2)
                tty->clear(TTY_LINE);
            else {
                kprintf("tty: unsupported J mode %d\n", args[0]);
            }
        } else {
            args_index++;

            kprintf("tty: unsupported code character: %c, nb_args=%d, args={", c, args_index);
            for (int i = 0; i < MIN(args_index, MAX_NB_ARGS); i++) kprintf(" %d", args[i]);
            kprintf(" }\n");
        }
        esc_mode = STATE_NORMAL;
        return;
    } else if (esc_mode == STATE_HASH) {
        if (c == '8') {
            tty->clear(TTY_PAGE_E);
        } else
            kprintf("tty: unsupported hash character: %c\n", c);
        esc_mode = STATE_NORMAL;
        return;
    } else if (esc_mode == STATE_CHARSET_G0) {
        esc_mode = STATE_NORMAL;
        return;
    } else if (esc_mode == STATE_CHARSET_G1) {
        esc_mode = STATE_NORMAL;
        return;
    }

    if (c == 27) {
       esc_mode = STATE_ESCAPE;
       return;
    }

    if (c == 7) {
        tty->clear(TTY_FLASH);
        return;
    }

    tty->putc(c);
}

static int tty_write(__attribute((unused)) FileDescriptor * fd, const void * buf_, int size)
{
    const uint8_t * buf = buf_;
    for (int i = 0; i < size; i++)
        tty_process(buf[i]);
    return size;
}

/* cooked mode */

static RingBuffer cookedrb;

static char linebuf[2048];
static int linepos = 0;

static void drain()
{
    char buf[8];
    int size = kb_read(buf, 8, 0);
    if (size <= 0)
        return;
    for (int i = 0; i < size; i++) {
         int c = buf[i];
         if (c == '\b') {
             if (linepos > 0)
                 linepos--;
         } else {
             linebuf[linepos++] = c;
             if (c == 0x4 || c == '\n') {
                 ringbuffer_write(&cookedrb, linebuf, linepos);
                 linepos = 0;
             }
         }
    }
}

static char cookedrb_buffer[2048];

void tty_init()
{
    ringbuffer_init(&cookedrb, cookedrb_buffer, sizeof(cookedrb_buffer));
}

static int cooked_available()
{
    return ringbuffer_read_available(&cookedrb);
}

static int cooked_read(char * buf, int size)
{
    int i;
    if (!cooked_available())
        return -EAGAIN;
    for (i = 0; i < size && cooked_available(); i++) {
        buf[i] = ringbuffer_read1(&cookedrb);
        if (buf[i] == 0x4) /* end of xmit */
            return MAX(0, i - 1);
    }
    return i;

}

static int tty_read(__attribute((unused)) FileDescriptor * fd, void * buf, int size)
{
    if ((tty_lflag & ICANON)) {
        drain();
        return cooked_read(buf, size);
    }
    return kb_read(buf, size, 1);
}

static int tty_read_available(__attribute((unused)) const FileDescriptor * fd)
{
    if ((tty_lflag & ICANON)) {
        drain();
        return cooked_available();
    }
    return kb_available();
}

static int tty_tcgetpgrp(__attribute((unused)) FileDescriptor * fd)
{
    return tty_foreground_pgrp;
}

static int tty_tcsetpgrp(__attribute((unused)) FileDescriptor * fd, pid_t pgrp)
{
    tty_foreground_pgrp = pgrp;
    return 0;
}

static int tty_tcgetattr(FileDescriptor * fd, struct termios * termios_p)
{
    memset(termios_p, 0, sizeof(struct termios));
    termios_p->c_lflag = tty_lflag;
    termios_p->c_oflag = tty_oflag;
    return 0;
}

static int tty_tcsetattr(FileDescriptor * fd, int optional_actions, const struct termios * termios_p)
{
    tty_lflag = termios_p->c_lflag;
    tty_oflag = termios_p->c_oflag;
    kprintf("tcsetattr: c_iflag=0x%x c_oflag=0x%x\n", termios_p->c_iflag, termios_p->c_oflag);
    return 0;
}

/*
 * linux keyboard api
 */
#include <errno.h>
#include "linux/kd.h"
#include <sys/ioctl.h>

static int tty_ioctl(FileDescriptor * fd, int request, void * data)
{
    if (request == KDGKBTYPE) {
        *(int *)data = KB_101;
        return 0;
    } else if (request == KDGKBMODE) {
        *(int *)data = kb_rawmode ? K_MEDIUMRAW : K_UNICODE;
        return 0;
    } else if (request == KDSKBMODE) {
        kb_rawmode = (intptr_t)data == K_MEDIUMRAW;
        return 0;
    } else if (request == TIOCGWINSZ) {
        struct winsize * ws = data;
        ws->ws_row = tty->rows();
        ws->ws_col = tty->columns();
        return 0;
    }
    return -EINVAL;
}

const DeviceOperations tty_dio = {
    .write = tty_write,
    .read  = tty_read,
    .read_available  = tty_read_available,
    .ioctl = tty_ioctl,
    .tcgetpgrp = tty_tcgetpgrp,
    .tcsetpgrp = tty_tcsetpgrp,
    .tcgetattr = tty_tcgetattr,
    .tcsetattr = tty_tcsetattr,
};
