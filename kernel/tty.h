#include <stdint.h>
#include "vfs.h"

enum {
    TTY_LEFT = 0,
    TTY_RIGHT,
    TTY_UP,
    TTY_DOWN
};

enum {
    TTY_PAGE = 0,
    TTY_PAGE_E,
    TTY_PAGE_BEFORE,
    TTY_PAGE_AFTER,
    TTY_LINE,
    TTY_LINE_BEFORE,
    TTY_LINE_AFTER,
    TTY_FLASH
};

enum {
    TTY_UP_SCROLL = 0,
    TTY_DOWN_SCROLL,
    TTY_CRLF_SCROLL
};

typedef struct {
    int (*ready)(void);
    void (*putc)(int c);
    void (*clear)(int what);
    void (*set_pos)(int xx, int yy);
    void (*move_cursor)(int direction, int count);
    void (*move_cursor_scroll)(int direction);
    void (*scroll_region)(int top, int bottom);
    void (*save_cursor)(void);
    void (*restore_cursor)(void);
    void (*reset)(void);
    int (*rows)(void);
    int (*columns)(void);
} TTYCommands;

void fb_init(uintptr_t addr, uint32_t stride, uint32_t width, uint32_t height, uint32_t bpp);
void fb_init2(void);
extern const DeviceOperations fb_io;

void textmode_init(void);

extern const TTYCommands fb_commands;
extern const TTYCommands textmode_commands;

void kb_init(void);
int kb_available(void);
int kb_read(char * buf, int size, int process_end_of_xmit);
void kb_put(int c);
void kb_puts(char * s);
extern int kb_rawmode;

void tty_init(void);
extern const TTYCommands * tty;
extern int tty_foreground_pgrp;
extern int tty_oflag;
extern int tty_lflag;
extern int tty_inverse;
extern int tty_kbmode;
extern int tty_autowrap;
void tty_puts(const char * s);

extern const DeviceOperations tty_dio;
