#include <bsd/string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

static void vi_signal_handler(int sig) { }

enum { COMMAND = 0, INPUT, PROMPT };

typedef struct {
    char orig_path[PATH_MAX];
    struct winsize ws;
    int mode;
    char * buf;
    size_t size;
    int dirty;
    int origin;
    int cursor;
    char message[64];
    char prompt[64];
    unsigned int count;

    int redraw;
    int last_origin;
    int last_endp;
} State;

/* return < 0 if cursor on first line, > 0 if cursor on last line and last line spans final row */
static int render(State * st, uintptr_t * endp)
{
    int do_output = st->redraw || st->origin != st->last_origin || st->cursor >= st->size - 1;

    if (do_output) {
        st->redraw = 0;
        st->last_origin = st->origin;
        printf("\33[2J\033[1;1H");
    } else {
        printf("\33[%d;%dH", st->ws.ws_row, 1);
        printf("\33[2K");
    }

    int cursor_c = -1, cursor_r = -1;
    int p = st->origin;
    int r = 0;
    int c = 0;

    if (st->size == 0)
        cursor_r = cursor_c = 0;

    while (p < st->size) {
#define OUTPUT(x) if (do_output) printf("%c", x); c++; if (c >= st->ws.ws_col) { c = 0; r++; if (r >= st->ws.ws_row - 1) break; }
        int ch = st->buf[p];
        if (p == st->cursor) {
            cursor_c = c;
            cursor_r = r;
        }
        if (ch < 32) {
            OUTPUT('^');
            OUTPUT(ch + 67);
            if (ch == '\n' && c != 0) {
                if (do_output) printf("\n");
                c = 0;
                r++;
                if (r >= st->ws.ws_row - 1) break;
            }
        } else {
            OUTPUT(ch);
        }
        p++;
    }
    if (st->cursor >= st->size) {
        cursor_c = c;
        cursor_r = r;
    }
    int filler = 0;
    if (r < st->ws.ws_row - 1 && (st->size == 0 || (p > 0 && st->buf[p - 1] != '\n') || (st->cursor >= st->size && st->buf[st->cursor - 1] == '\n'))) {
        if (do_output) printf("\n");
        c = 0;
        r++;
    }
    while (r < st->ws.ws_row - 1) {
        if (do_output) printf("~\n");
        r++;
        filler = 1;
    }

    if (st->mode == COMMAND) {
        if (st->message[0])
            printf("\033[7m%s\033[0m", st->message);
    } else if (st->mode == INPUT) {
        printf("--INSERT--");
    } else if (st->mode == PROMPT) {
        printf("%s", st->prompt);
        cursor_r = st->ws.ws_row - 1;
        cursor_c = strlen(st->prompt);
    }

    if (st->mode != PROMPT) {
        printf(" (%d/%d)", st->cursor, (int)st->size);
        if (st->count) printf(" %d", st->count);
        if (cursor_r < 0 || cursor_c < 0) printf(" cursor off screen");
    }

    if (cursor_r >= 0 || cursor_c >= 0)
        printf("\33[%d;%dH", cursor_r+1, cursor_c+1);
    fflush(stdout);

    *endp = p;

    if (!filler && p > 0) {
        int i;
        for (i = p - 1; i >= 0 && st->buf[i] != '\n'; i--) ;
        if (st->cursor > i)
            return 1;
    }
    return cursor_r == 0 ? -1 : 0;
}

static void delete(State * st)
{
    if (!st->size)
        return;
    if (st->size == 1) {
        st->cursor = 0;
        st->size = 0;
    } else if (st->size - st->cursor > 0) {
        memmove(st->buf + st->cursor, st->buf + st->cursor + 1, st->size - st->cursor - 1);
        st->size--;
    }
    st->dirty = 1;
    st->redraw = 1;
}

static int findlinestart(const State * st, int start)
{
    for (start--; start >= 0 && st->buf[start] != '\n'; start--) ;
    return start + 1;
}

static int findlineend(const State * st, int start)
{
    if (!st->size)
        return 0;
    for (; start < st->size && st->buf[start] != '\n'; start++) ;
    if (start == st->size)
        start--;
    return start;
}

static int getcursorchar(const State * st)
{
    return st->cursor - findlinestart(st, st->cursor);
}

static void deleteline(State * st)
{
    if (!st->size)
        return;
    int start  = findlinestart(st, st->cursor);
    int end = findlineend(st, st->cursor);
    int size = end - start + 1;
    memmove(st->buf + start, st->buf + end + 1, st->size - size);
    st->size -= size;
    st->cursor = start;
    if (st->size && st->cursor >= st->size)
        st->cursor = st->size - 1;

    st->dirty = 1;
    st->redraw = 1;
}

static void scrollup(State * st, int count)
{
    while (count--) {
        int j;
        for (j = st->origin - 2; j >= 0 && st->buf[j] != '\n'; j--) ;
        st->origin = MAX(0, j + 1);
    }
}

static void scrolldown(State * st, int count)
{
    if (!st->size)
        return;
    while (count--) {
        int j;
        for (j = st->origin; j < st->size && st->buf[j] != '\n'; j++);
        if (j >= st->size - 1)
            for (j = st->size - 2; j >= 0 && st->buf[j] != '\n'; j--) ;
        st->origin = j + 1;
    }
}

static void lineup(State * st, int cursor_where)
{
    int cursorchar = getcursorchar(st);
    int i;
    for (i = st->cursor - cursorchar - 2; i >= 0 && st->buf[i] != '\n'; i--) ;
    i++;
    if (i < 0)
        return;

    int len;
    for (len = 0; i + len < st->size && st->buf[i + len] != '\n'; len++);
    cursorchar = MIN(cursorchar, len);

    st->cursor = MAX(0, i + cursorchar);
    if (cursor_where < 0)
        scrollup(st, 1);
}

static void linedown(State * st, int cursor_where)
{
    if (!st->size)
        return;

    int cursorchar = getcursorchar(st);
    int i;

    if (st->buf[st->cursor] == '\n') {
        i = st->cursor;
    } else {
        for (i = st->cursor + 1; i < st->size && st->buf[i] != '\n'; i++) ;
        if (i >= st->size - 1)
            return;
    }

    int len;
    for (len = 0; i + 1 + len < st->size && st->buf[i + 1 + len] != '\n'; len++);
    cursorchar = MIN(cursorchar, len);

    st->cursor = MIN(st->size - 1, i + 1 + cursorchar);
    if (cursor_where > 0)
        scrolldown(st, 1);
}

static void flash()
{
    printf("\07");
    fflush(stdout);
}

static int getchar_escape()
{
    int escape = 0;
    int modify;
    int push_char;

    while(1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        struct timeval timeout = {0, 100000};
        int n = select(STDIN_FILENO + 1, &rfds, NULL, NULL, escape ? &timeout : NULL);
        if (n == -1) {
            perror("select");
            continue;
        }
        if (n == 0) {
            if (escape)
                return 27; //escape
        } else {
            char c;
            read(STDIN_FILENO, &c, 1);
            if (escape) {
                modify++;
                if (modify == 1) {
                    continue;
                } else if (modify == 2) {
                    switch(c) {
                    case 51: push_char = 'x'; break; //delete
                    case 53: push_char = 2; break; //pgup
                    case 54: push_char = 6; break; //pgdn
                    case 65: return 'k'; //up arrow
                    case 66: return 'j'; //down arrow
                    case 67: return 'l'; //right arrow
                    case 68: return 'h'; //left arrow
                    case 70: return '$'; //end
                    case 72: return '0'; //home
                    default:
                       fprintf(stderr, "[unsupported escape code %d]\n", c);
                       getchar();
                       return c;
                    }
                } else { //modify==3
                    return push_char;
                }
                continue;
            } else {
                if (c == 27) {
                   escape = 1;
                   modify = 0;
                   continue;
                } else
                   return c;
            }
        }
    }
}

static int save_file(State * st, const char * path)
{
    int fd = open(path, O_WRONLY|O_CREAT, 0666);
    if (fd == -1) {
        snprintf(st->message, sizeof(st->message), "\"%s\" write error: %s", path, strerror(errno));
        return -1;
    }
    if (write(fd, st->buf, st->size) == -1) {
        snprintf(st->message, sizeof(st->message), "\"%s\" write error: %s", path, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);

    snprintf(st->message, sizeof(st->message), "\"%s\" written", path);
    st->dirty = 0;
    return 0;
}

static int reallocbuf(State * st, int extra)
{
    char * newbuf = realloc(st->buf, st->size + extra);
    if (!newbuf) {
        snprintf(st->message, sizeof(st->message), "ENOMEM");
        st->mode = COMMAND;
        return -1;
    }
    st->buf = newbuf;
    return 0;
}

static void search(State * st, int endp)
{
    if (!st->size || st->cursor == st->size - 1)
        return;
    if (reallocbuf(st, 1) < 0)
        return;
    st->buf[st->size] = 0;
    char * s = strstr(st->buf + st->cursor + 1, st->prompt + 1);
    if (!s) {
        snprintf(st->message, sizeof(st->message), "not found");
        return;
    }
    uintptr_t pos = (uintptr_t)(s - st->buf);
    if (pos >= st->origin && pos < endp) {
        st->cursor = pos;
        return;
    }
    st->cursor = pos;
    for (st->origin = pos - 1; st->origin >= 0 && st->buf[st->origin] != '\n'; st->origin--) ;
    st->origin++;
}

static int vi_main(int argc, char ** argv, char ** envp)
{
    State st;
    if (argc > 1) {
        strlcpy(st.orig_path, argv[1], sizeof(st.orig_path));
        int fd = open(argv[1], O_RDONLY);
        if (fd == -1) {
            perror("open");
            return -1;
        }
        st.size = lseek(fd, 0, SEEK_END);
        st.buf = malloc(st.size);
        if (!st.buf) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        lseek(fd, 0, SEEK_SET);
        read(fd, st.buf, st.size);
        close(fd);
    } else {
        st.orig_path[0] = 0;
        st.size = 0;
        st.buf = malloc(0);
        if (!st.buf) {
            perror("malloc");
            return EXIT_FAILURE;
        }
    }

    struct termios old, raw;
    tcgetattr(STDIN_FILENO, &old);
    signal(SIGINT, vi_signal_handler);
    raw = old;
    raw.c_lflag &= ~(ECHO|ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    ioctl(STDIN_FILENO, TIOCGWINSZ, &st.ws);

    st.origin = 0;
    st.cursor = 0;
    st.dirty = 0;
    st.message[0] = 0;
    st.count = 0;

    st.mode = COMMAND;
    st.redraw = 1;
    int repeat = 0;
    while(1) {
        uintptr_t endp;
        int cursor_where = render(&st, &endp);
        char c = getchar_escape();
        st.message[0] = 0;
        if (st.mode == COMMAND) {
            if ((c >= '1' && c <= '9') || (c == '0' && st.count > 0)) {
                st.count = st.count * 10 + c - '0';
                continue;
            }
            st.count = MAX(1, st.count);

            if (c == 'd') {
                repeat++;
                if (repeat == 2) {
                    while(st.count-- && st.size)
                        deleteline(&st);
                    repeat = 0;
                }
            } else {
                repeat = 0;
            }

            if (c == 'd') {
                /* avoid key unsupported message */
            } else if (c == 'i') {
                st.mode = INPUT;
            } else if (c == 'a') {
                if (st.size > 0 && st.buf[st.cursor] != '\n')
                    st.cursor++;
                st.mode = INPUT;
            } else if (c == ':' || c == '/') {
                st.mode = PROMPT;
                st.prompt[0] = c;
                st.prompt[1] = 0;
            } else if (c == '0') {
                st.cursor = findlinestart(&st, st.cursor);
            } else if (c == '$') {
                st.cursor = findlineend(&st, st.cursor);
            } else if (c == 'h') {
                while(st.count-- && getcursorchar(&st) > 0)
                    st.cursor--;
            } else if (c == 'l' || c == ' ') {
                while(st.count-- && st.cursor < findlineend(&st, st.cursor))
                    st.cursor++;
            } else if (c == 'k') {
                while(st.count--)
                    lineup(&st, cursor_where);
            } else if (c == 'j') {
                while(st.count--)
                    linedown(&st, cursor_where);
            } else if (c == 2) {
                scrollup(&st, st.ws.ws_row - 2);
                st.cursor = st.origin;
            } else if (c == 6) {
                scrolldown(&st, st.ws.ws_row - 2);
                st.cursor = st.origin;
            } else if (c == 'b') {
                while (st.count-- && st.size) {
                    st.cursor--;
                    while (st.cursor > 0 && isspace(st.buf[st.cursor])) st.cursor--;
                    while (st.cursor > 0 && !isspace(st.buf[st.cursor])) st.cursor--;
                    st.cursor++;
                }
            } else if (c == 12) {
                st.redraw = 1;
            } else if (c == 'w') {
                while (st.count-- && st.size) {
                    while (st.cursor < st.size && !isspace(st.buf[st.cursor])) st.cursor++;
                    while (st.cursor < st.size && isspace(st.buf[st.cursor])) st.cursor++;
                    if (st.cursor >= st.size)
                        st.cursor = st.size - 1;
                }
            } else if (c == 'G') {
                if (st.size) {
                    st.cursor = st.size - 1;
                    st.origin = findlinestart(&st, st.cursor);
                }
            } else if (c == 27) {
                flash();
            } else if (c == '\b' || c == 127) { //backspace
                if (st.count-- && st.cursor > 0) {
                    st.cursor--;
                    delete(&st);
                }
            } else if (c == 'x') {
                while (st.count-- && st.size) {
                    delete(&st);
                    if (st.size && st.cursor == st.size)
                        st.cursor--;
                }
            } else if (c == 'n') {
                if (st.prompt[0] == '/')
                    while (st.count--)
                        search(&st, endp);
            } else {
                snprintf(st.message, sizeof(st.message), "%d key unsupported", c);
            }

            st.count = 0;
        } else if (st.mode == INPUT) {
            if (c == 27) {
                if (st.cursor > 0)
                    st.cursor--;
                st.mode = COMMAND;
            } else if (c == '\b' || c == 127) { //backspace
                if (st.cursor > 0) {
                    st.cursor--;
                    delete(&st);
                }
            } else {
                if (reallocbuf(&st, 1) < 0)
                    continue;
                if (st.cursor < st.size)
                    memmove(st.buf + st.cursor + 1, st.buf + st.cursor, st.size - st.cursor);
                st.size += 1;
                st.buf[st.cursor++] = c;
                st.dirty = 1;
                st.redraw = 1;
                if (c == '\n' && cursor_where > 0)
                    scrolldown(&st, 1);
            }
        } else if (st.mode == PROMPT) {
            if (c == 27) {
                st.mode = COMMAND;
            } else if (c == '\b' || c == 127) { //backspace
                if (strlen(st.prompt) > 0)
                    st.prompt[strlen(st.prompt) - 1] = 0;
            } else if (c == '\n') {
                if (st.prompt[0] == ':') {
                    for (char * s = st.prompt + 1; *s; s++) {
                        if (*s == 'q') {
                            if (st.dirty && s[1] != '!') {
                                snprintf(st.message, sizeof(st.message), "dirty buffer");
                                break;
                            }
                            goto quit;
                        } else if (*s == 'w') {
                            const char * path = (s[1] == ' ' && s[2]) ? s + 2 : NULL;
                            if (!st.orig_path[0] && !path) {
                                snprintf(st.message, sizeof(st.message), "no file name");
                                break;
                            }
                            if (save_file(&st, path ? path : st.orig_path) < 0)
                                break;
                            if (path)
                                break;
                        } else if (*s == 'x') {
                            if (st.orig_path[0]) {
                                if (save_file(&st, st.orig_path) < 0)
                                    break;
                            }
                            goto quit;
                        } else if (*s == '0') {
                            st.origin = 0;
                            st.cursor = 0;
                        } else {
                            snprintf(st.message, sizeof(st.message), "%c command unsupported", *s);
                            break;
                        }
                    }
                } else if (st.prompt[0] == '/') {
                    search(&st, endp);
                }
                st.mode = COMMAND;
            } else {
                snprintf(st.prompt + strlen(st.prompt), sizeof(st.prompt) - strlen(st.prompt), "%c", c);
            }
        }
    }

quit:
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);
    printf("\33[2J\033[1;1H");
    fflush(stdout);
    return EXIT_SUCCESS;
}
