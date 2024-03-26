#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "tty.h"
#include "utils.h"

static uint8_t * fb_addr;
static unsigned int fb_stride;
static int fb_width;
static int fb_height;
static unsigned int fb_bpp;

static int x = 0;
static int y = 0;
static int scroll_top = 0;
static int scroll_bottom = 0;
static int saved_x = 0;
static int saved_y = 0;

static void page_xor()
{
    if (fb_bpp == 1) {
        for (unsigned int j = 0; j < fb_height; j++)
            for (unsigned int i = 0; i < fb_width / 8; i++)
                fb_addr[j*fb_stride + i] ^= 0xFF;
    } else {
    for (unsigned int j = 0; j < fb_height; j++)
        for (unsigned int i = 0; i < fb_width; i++)
            for (unsigned int k = 0; k < fb_bpp/8; k++)
                fb_addr[j*fb_stride + i*fb_bpp/8 + k] ^= 0xFF;
    }
}

static void cursor_xor()
{
    if (fb_bpp == 1) {
        for (unsigned int j = 0; j < 8; j++)
            fb_addr[(y + j) * fb_stride + x/8] ^= 0xFF;
    } else {
    for (unsigned int j = 0; j < 8; j++)
        for (unsigned int i = 0; i < 8; i++)
            for (unsigned int k = 0; k < fb_bpp/8; k++)
                fb_addr[(y + j) * fb_stride + (x + i) * fb_bpp/8 + k] ^= 0xFF;
    }
}

static void fb_reset()
{
    scroll_top = 0;
    scroll_bottom = fb_height - 8;
}

void fb_init(uint64_t addr, uint32_t stride, uint32_t width, uint32_t height, uint32_t bpp)
{
    fb_addr = (uint8_t *)(uint32_t)addr;
    fb_stride = stride;
    fb_width = width;
    fb_height = height;
    fb_bpp = bpp;

    fb_reset();

    memset(fb_addr, 0xFF, fb_stride * fb_height);
    cursor_xor();

    kprintf("fb_init 0x%llx, 0x%x, stride=%d, height=%d\n", addr, fb_addr, stride, height);
}

#include "font8x8_basic.h"

static int swap8(int c)
{
    int ret = 0;
    for (int i = 0; i < 8; i++)
        ret |= !!(c & (1 << i)) << (7 - i);
    return ret;
}

static void clear_e()
{
    if (fb_bpp == 1) {
        for (int yy = 0; yy < fb_height; yy += 8)
            for (int xx = 0; xx < fb_width; xx += 8)
                for (unsigned int j = 0; j < 8; j++)
                    fb_addr[(yy + j) * fb_stride + xx/8] = swap8(font8x8_basic['E'][j]) ^ 0xFF;
    } else {
    for (int yy = 0; yy < fb_height; yy += 8)
        for (int xx = 0; xx < fb_width; xx += 8)
            for (unsigned int j = 0; j < 8; j++)
                for (unsigned int i = 0; i < 8; i++)
                    for (unsigned int k = 0; k < fb_bpp/8; k++)
                        fb_addr[(yy + j) * fb_stride + (xx + i) * fb_bpp/8 + k] = font8x8_basic['E'][j] & (1 << i) ? 0x00 : 0xff;
    }
}

static void clear_line(int xx, int yy, int width)
{
    for (unsigned int j = 0; j < 8; j++)
        memset(fb_addr + (yy+j)*fb_stride + xx * fb_bpp/8, 0xFF, width * fb_bpp/8);
}

static void fb_clear(int what)
{
    cursor_xor();
    switch (what) {
    case TTY_PAGE:
        memset(fb_addr, 0xFF, fb_height * fb_stride);
        break;
    case TTY_PAGE_E:
        clear_e();
        break;
    case TTY_PAGE_BEFORE:
        if (y >= 8)
            memset(fb_addr, 0xFF, y * fb_stride);
        clear_line(0, y, x + 8);
        break;
    case TTY_PAGE_AFTER:
        clear_line(x, y, fb_width - x);
        memset(fb_addr + (y + 8)*fb_stride, 0xFF, (fb_height - y - 8) * fb_stride);
        break;
    case TTY_LINE:
        clear_line(0, y, fb_width);
        break;
    case TTY_LINE_BEFORE:
        clear_line(0, y, x + 8);
        break;
    case TTY_LINE_AFTER:
        clear_line(x, y, fb_width - x);
        break;
    case TTY_FLASH:
        page_xor();
        page_xor();
        break;
    default:
        kprintf("fb_clear %d not implemented\n", what);
        break;
    }
    cursor_xor();
}

static void fb_set_pos(int xx, int yy)
{
    cursor_xor();
    x = MAX(0, MIN(xx * 8, fb_width - 8));
    y = MAX(0, MIN(yy * 8, fb_height - 8));
    cursor_xor();
}

static void check_scroll_up()
{
    if (y == scroll_top - 8) {
        memmove(fb_addr + (scroll_top + 8) * fb_stride, fb_addr + scroll_top*fb_stride, (scroll_bottom - scroll_top) * fb_stride);
        memset(fb_addr + scroll_top * fb_stride, 0xFF, fb_stride * 8);
        y = scroll_top;
    } else if (y < 0)
        y = 0;
}

static void check_scroll_down()
{
    if (y == scroll_bottom + 8) {
        memcpy(fb_addr + scroll_top*fb_stride, fb_addr + (scroll_top + 8) * fb_stride, (scroll_bottom - scroll_top) * fb_stride);
        memset(fb_addr + scroll_bottom * fb_stride, 0xFF, fb_stride * 8);
        y = scroll_bottom;
    } else if (y >= fb_height)
        y = fb_height - 8;
}

static void fb_putc(int c)
{
    cursor_xor();

    if (c == '\r') {
        x = 0;
    } else if (c == '\n') {
        if (tty_oflag & ONLCR) x = 0;
        y += 8;
    } else if (c == '\b') {
        x = MAX(0, x - 8);
    } else if (c == '\t') {
        x += 0x40; // 8*8
        x &= ~0x3f;
        if (x >= fb_width) {
            if (tty_autowrap) {
                x -= fb_width;
                y += 8;
            } else
                x = fb_width - 8;
        }
    } else if (c < 32 || c > 127) {
        /* ignore */
    } else {

        if (c >= 128)
            c = '*';

        if (fb_bpp == 1) {
            for (unsigned int j = 0; j < 8; j++)
                fb_addr[(y + j) * fb_stride + x/8] = swap8(font8x8_basic[c][j]) ^ 0xFF ^ tty_inverse;
        } else {
        for (unsigned int j = 0; j < 8; j++)
            for (unsigned int i = 0; i < 8; i++)
                for (unsigned int k = 0; k < fb_bpp/8; k++)
                    fb_addr[(y + j) * fb_stride + (x + i) * fb_bpp/8 + k] = (font8x8_basic[c][j] & (1 << i) ? 0x00 : 0xff) ^ tty_inverse;
        }
        x += 8;
        if (x >= fb_width) {
            if (tty_autowrap) {
                x = 0;
                y += 8;
            } else
                x = fb_width - 8;
        }
    }

    check_scroll_down();

    cursor_xor();
}

uint32_t fb_get_base(void)
{
    return (uint32_t)fb_addr;
}

uint32_t fb_get_end(void)
{
    return (uint32_t)fb_addr + fb_stride * fb_height;
}

static void fb_move_cursor(int direction, int count)
{
    count = MAX(1, count);
    cursor_xor();
    switch(direction) {
    case TTY_LEFT:  x -= count * 8; break;
    case TTY_RIGHT: x += count * 8; break;
    case TTY_UP:    y -= count * 8; break;
    case TTY_DOWN:  y += count * 8; break;
    }
#if 0
    if (x < 0)  { x += fb_width; y -= 8; }
    if (x > fb_width) { x -= fb_width; y += 8; }
#endif
    x = MAX(0, MIN(x, fb_width - 8));
    y = MAX(0, MIN(y, fb_height - 8));
    cursor_xor();
}

static void fb_move_cursor_scroll(int direction)
{
    cursor_xor();
    switch(direction) {
    case TTY_UP_SCROLL:
        y -= 8;
        check_scroll_up();
        break;
    case TTY_DOWN_SCROLL:
        y += 8;
        check_scroll_down();
        break;
    case TTY_CRLF_SCROLL:
        x = 0;
        y += 8;
        check_scroll_down();
        break;
    }
    cursor_xor();
}

static void fb_scroll_region(int top, int bottom)
{
    if (top && bottom && bottom > top) {
        scroll_top    = MAX(0, MIN((top - 1) * 8, fb_height - 8));
        scroll_bottom = MAX(0, MIN((bottom - 1) * 8, fb_height - 8));
    } else {
        scroll_top = 0;
        scroll_bottom = fb_height - 8;
    }
    fb_set_pos(0, 0);
}

static void fb_save_cursor()
{
    saved_x = x;
    saved_y = y;
}

static void fb_restore_cursor()
{
    cursor_xor();
    x = saved_x;
    y = saved_y;
    cursor_xor();
}

static int fb_rows()
{
    return fb_height / 8;
}

static int fb_columns()
{
    return fb_width / 8;
}

const TTYCommands fb_commands = {
    .putc = fb_putc,
    .clear = fb_clear,
    .set_pos = fb_set_pos,
    .move_cursor = fb_move_cursor,
    .move_cursor_scroll = fb_move_cursor_scroll,
    .scroll_region = fb_scroll_region,
    .save_cursor = fb_save_cursor,
    .restore_cursor = fb_restore_cursor,
    .reset = fb_reset,
    .rows = fb_rows,
    .columns = fb_columns,
};


/* linux fb api */

#include <linux/fb.h>
#include <errno.h>
#include <bsd/string.h>

static int fb_ioctl(FileDescriptor * fd, int request, void * data)
{
    if (request == FBIOGET_FSCREENINFO) {
        struct fb_fix_screeninfo * p = data;
        strlcpy(p->id, "fb", sizeof(p->id));
        p->line_length = fb_stride;
        p->smem_len = fb_stride * fb_height;
    } else if (request == FBIOGET_VSCREENINFO) {
        struct fb_var_screeninfo * p = data;
        p->xres = p->xres_virtual = fb_width;
        p->yres = p->yres_virtual = fb_height;
        p->xoffset = 0;
        p->yoffset = 0;
        p->bits_per_pixel = fb_bpp;
        p->red.offset = 16;
        p->red.length = 8;
        p->green.offset = 8;
        p->green.length = 8;
        p->blue.offset = 0;
        p->blue.length = 8;
        p->transp.offset = 24;
        p->transp.length = 8;
        p->grayscale = 0;
    } else
        return -EINVAL;
    return 0;
}

#include <os/syscall.h>

static int fb_mmap(FileDescriptor * fd, struct os_mmap_request * req)
{
    req->addr = fb_addr;
    return 0;
}

static int fb_write(FileDescriptor * fd, const void * buf, int size)
{
    size_t sz = MIN(fb_height * fb_stride - fd->pos, size);

    memcpy(fb_addr + fd->pos, buf, sz);

    fd->pos += sz;
    return sz;
}

static off_t fb_lseek(FileDescriptor * fd, off_t offset, int whence)
{
    off_t size = fb_height * fb_stride;
    if (whence == SEEK_END)
        fd->pos = size;
    else if (whence == SEEK_CUR)
        fd->pos = MIN(size, MAX(0, fd->pos + offset));
    else if (whence == SEEK_SET)
        fd->pos = MIN(size, MAX(0, offset));

    return fd->pos;
}

const DeviceOperations fb_io = {
    .write = fb_write,
    .lseek = fb_lseek,
    .ioctl = fb_ioctl,
    .mmap = fb_mmap,
};
