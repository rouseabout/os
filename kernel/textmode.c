#include <string.h>
#include "utils.h"
#include "tty.h"
#include <termios.h>
#include <ctype.h>

static int x = 0;
static int y = 0;
static int scroll_top = 0;
static int scroll_bottom = 24;
static int saved_x = 0;
static int saved_y = 0;

void textmode_init()
{
    int l;
    outb(0x3D4, 14);
    l = inb(0x3D5) << 8;
    outb(0x3D4, 15);
    l |= inb(0x3D5);
    y = l / 80;
    x = l % 80;
}

static void xor(uint8_t * dst, int size)
{
    for (unsigned int i = 0; i < size; i++)
        *(dst + i*2 + 1) ^= 0x77;
}

static void clear2(uint8_t * dst, int size, int c)
{
    for (unsigned int i = 0; i < size; i++) {
        *(dst + i*2 + 0) = c;
        *(dst + i*2 + 1) = 0x07;
    }
}

static void clear(uint8_t * dst, int size)
{
    clear2(dst, size, ' ');
}

static void update_cursor()
{
    int l = y * 80 + x;
    outb(0x3D4, 14);
    outb(0x3D5, l >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, l);
}

static void textmode_clear(int what)
{
    uint8_t * ptr = (uint8_t *)0xb8000;

    switch (what) {
    case TTY_PAGE:
        clear(ptr, 80*25);
        break;
    case TTY_PAGE_E:
        clear2(ptr, 80*25, 'E');
        break;
    case TTY_PAGE_BEFORE:
        clear(ptr, 80*y + x + 1);
        break;
    case TTY_PAGE_AFTER:
        clear(ptr + y*80*2 + x*2, (80 - x) + (24 - y)*80);
        break;
    case TTY_LINE:
        clear(ptr + y*80*2, 80);
        break;
    case TTY_LINE_BEFORE:
        clear(ptr + y*80*2, x + 1);
        break;
    case TTY_LINE_AFTER:
        clear(ptr + y*80*2 + x*2, 80 - x);
        break;
    case TTY_FLASH:
        xor(ptr, 80*25);
        xor(ptr, 80*25);
        break;
    }
}

static void textmode_set_pos(int xx, int yy)
{
     x = MAX(0, MIN(79, xx));
     y = MAX(0, MIN(24, yy));
     update_cursor();
}

static void check_scroll_down()
{
    uint8_t * ptr = (uint8_t *)0xb8000;

    if (y == scroll_bottom + 1) {

        memcpy(ptr + scroll_top*80*2, ptr + (scroll_top + 1)*80*2, (scroll_bottom - scroll_top)*80*2);
        clear(ptr + 80*2*scroll_bottom, 80);

        y = scroll_bottom;
    } else if (y >= 25)
        y = 24;
}

static void check_scroll_up()
{
    uint8_t * ptr = (uint8_t *)0xb8000;

    if (y == scroll_top - 1) {
        memmove(ptr + (scroll_top + 1)*80*2, ptr + scroll_top*80*2, (scroll_bottom - scroll_top)*80*2);
        clear(ptr + 80*2*scroll_top, 80);
        y = scroll_top;
    } else if (y < 0)
        y = 0;
}

static void textmode_putc(int ch)
{
    uint8_t * ptr = (uint8_t *)0xb8000;

    if (ch == '\r') {
        x = 0;
    } else if (ch == '\n') {
        y++;
        if (tty_oflag & ONLCR) x = 0;
    } else if (ch == '\b') {
        x = MAX(0, x - 1);
    } else if (ch == '\t') {
        x += 8;
        x &= ~7;
        if (x >= 80) {
            if (tty_autowrap) {
                y++;
                x -= 80;
            } else
                x = 79;
        };
    } else if (ch < 32 || ch > 127) {
        /* ignore */
    } else {
        ptr[y * 80 * 2 + x * 2 + 0] = ch;
        ptr[y * 80 * 2 + x * 2 + 1] = !tty_inverse ? 0x07 : 0x70;
        x++;
        if (x==80) {
            if (tty_autowrap) {
                y++;
                x = 0;
            } else
                x = 79;
        };
    }

    check_scroll_down();

    update_cursor();
}

static void textmode_move_cursor(int direction, int count)
{
    count = MAX(1, count);
    switch(direction) {
    case TTY_LEFT:  x -= count; break;
    case TTY_RIGHT: x += count; break;
    case TTY_UP:    y -= count; break;
    case TTY_DOWN:  y += count; break;
    }
    /*FIXME: when terminal cursors hits column 80 and wraps around,
          it is possible to move LEFT backwards to the previous line */
#if 0
    if (x < 0)  { x += 80; y--; }
    if (x > 80) { x -= 80; y++; }
#endif
    x = MAX(0, MIN(79, x));
    y = MAX(0, MIN(24, y));
    update_cursor();
}

static void textmode_move_cursor_scroll(int direction)
{
    switch(direction) {
    case TTY_UP_SCROLL:
        y--;
        check_scroll_up();
        break;
    case TTY_DOWN_SCROLL:
        y++;
        check_scroll_down();
        break;
    case TTY_CRLF_SCROLL:
        x = 0;
        y++;
        check_scroll_down();
        break;
    }
    update_cursor();
}

static void textmode_scroll_region(int top, int bottom)
{
    if (top && bottom && bottom > top) {
        scroll_top = MAX(0, MIN(24, top - 1));
        scroll_bottom = MAX(0, MIN(24, bottom - 1));
    } else {
        scroll_top = 0;
        scroll_bottom = 24;
    }
    x = y = 0;
    update_cursor();
}

static void textmode_save_cursor()
{
    saved_x = x;
    saved_y = y;
}

static void textmode_restore_cursor()
{
    x = saved_x;
    y = saved_y;
    update_cursor();
}

static void textmode_reset()
{
    scroll_top = 0;
    scroll_bottom = 24;
}

static int textmode_rows()
{
    return 25;
}

static int textmode_columns()
{
    return 80;
}

const TTYCommands textmode_commands = {
    .putc = textmode_putc,
    .clear = textmode_clear,
    .set_pos = textmode_set_pos,
    .move_cursor = textmode_move_cursor,
    .move_cursor_scroll = textmode_move_cursor_scroll,
    .scroll_region = textmode_scroll_region,
    .save_cursor = textmode_save_cursor,
    .restore_cursor = textmode_restore_cursor,
    .reset = textmode_reset,
    .rows = textmode_rows,
    .columns = textmode_columns,
};
