#include <bsd/string.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
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
} State;

/* return < 0 if cursor on first line, > 0 if cursor on last line and last line spans final row */
static int render(State * st)
{
    printf("\33[2J\033[1;1H");
    int cursor_c = -1, cursor_r = -1;

    int p = st->origin;
    int r = 0;
    int c = 0;

    if (st->size == 0)
        cursor_r = cursor_c = 0;

    while (p < st->size) {
#define OUTPUT(x) printf("%c", x); c++; if (c >= st->ws.ws_col) { c = 0; r++; if (r >= st->ws.ws_row - 1) break; }
        int ch = st->buf[p];
        if (p == st->cursor) {
            cursor_c = c;
            cursor_r = r;
        }
        if (ch < 32) {
            OUTPUT('^');
            OUTPUT(ch + 67);
            if (ch == '\n' && c != 0) {
                printf("\n");
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
        printf("\n");
        c = 0;
        r++;
    }
    while (r < st->ws.ws_row - 1) {
        printf("~\n");
        r++;
        filler = 1;
    }
    if (st->mode == COMMAND) {
        if (st->message[0])
            printf("\033[7m%s\033[0m", st->message);
    } else if (st->mode == INPUT) {
        printf("--INSERT--");
    } else if (st->mode == PROMPT) {
        printf(":%s", st->prompt);
        cursor_r = r;
        cursor_c = 1 + strlen(st->prompt);
    }

    printf(" (%d/%d) %s", st->cursor, (int)st->size, cursor_r < 0 || cursor_c < 0 ? " cursor off screen" : "");

    if (cursor_r >= 0 || cursor_c >= 0)
        printf("\33[%d;%dH", cursor_r+1, cursor_c+1);
    fflush(stdout);

    if (!filler && p > 0) {
        int i;
        for (i = p - 1; i >= 0 && st->buf[i] != '\n'; i--) ;
        printf("i=%d",i);
        if (st->cursor > i)
            return 1;
    }
    return cursor_r == 0 ? -1 : 0;
}

static int getcursorchar(const State * st)
{
    int i;
    for (i = st->cursor - 1; i >= 0 && st->buf[i] != '\n'; i--) ;
    //printf("\n\n\n\n[cursor=%d, i=%d, delta=%d]\n", st.cursor, i, st.cursor - i - 1);
    return st->cursor - i - 1;
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
}

static void scrollup(State * st)
{
    int j;
    for (j = st->origin - 2; j >= 0 && st->buf[j] != '\n'; j--) ;
    st->origin = MAX(0, j + 1);
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
        snprintf(st->message, sizeof(st->message), "\"%s\" write error", path);
        return -1;
    }
    write(fd, st->buf, st->size);
    close(fd);

    snprintf(st->message, sizeof(st->message), "\"%s\" written", path);
    st->dirty = 0;
    return 0;
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

    st.mode = COMMAND;
    while(1) {
        int cursor_where = render(&st);
        char c = getchar_escape();
        st.message[0] = 0;
        if (st.mode == COMMAND) {
            if (c == 'i') {
                st.mode = INPUT;
            } else if (c == 'a') {
                if (st.size > 0)
                    st.cursor++;
                st.mode = INPUT;
            } else if (c == ':') {
                st.mode = PROMPT;
                st.prompt[0] = 0;
            } else if (c == '0') {
                st.cursor -= getcursorchar(&st);
            } else if (c == '$') {
                if (st.size > 0) {
                    for (; st.cursor < st.size && st.buf[st.cursor] != '\n'; st.cursor++) ;
                    if (st.cursor == st.size)
                        st.cursor = st.size - 1;
                }
            } else if (c == 'h') {
                int cursorchar = getcursorchar(&st);
                if (cursorchar > 0)
                    st.cursor--;
                else
                    flash();
            } else if (c == 'l') {
                if (st.size && st.cursor < st.size - 1 && st.buf[st.cursor] != '\n')
                    st.cursor++;
                else
                    flash();
            } else if (c == 'k') {
                int cursorchar = getcursorchar(&st);
                int i;
                for (i = st.cursor - cursorchar - 2; i >= 0 && st.buf[i] != '\n'; i--) ;
                i++;
                if (i < 0) {
                    flash();
                    continue;
                }

                int len;
                for (len = 0; i + len < st.size && st.buf[i + len] != '\n'; len++);
                cursorchar = MIN(cursorchar, len);

                st.cursor = MAX(0, i + cursorchar);

                if (cursor_where < 0)
                    scrollup(&st);

            } else if (c == 'j') {
                int cursorchar = getcursorchar(&st);
                int i;
                if (!st.size) {
                    continue;
                } else if (st.buf[st.cursor] == '\n') {
                    i = st.cursor;
                } else {
                    for (i = st.cursor + 1; i < st.size && st.buf[i] != '\n'; i++) ;
                    if (i >= st.size - 1) {
                        flash();
                        continue;
                    }
                }

                int len;
                for (len = 0; i + 1 + len < st.size && st.buf[i + 1 + len] != '\n'; len++);
                cursorchar = MIN(cursorchar, len);

                st.cursor = MIN(st.size - 1, i + 1 + cursorchar);

                if (cursor_where > 0) {
                    int j;
                    for (j = st.origin; j < st.size && st.buf[j] != '\n'; j++);
                    st.origin = j + 1;
                }
            } else if (c == 27) {
                flash();
            } else if (c == '\b' || c == 127) { //backspace
                if (st.cursor > 0) {
                    st.cursor--;
                    delete(&st);
                }
            } else if (c == 'd' || c == 'x') {
                delete(&st);
                if (st.size && st.cursor == st.size)
                    st.cursor--;
            } else {
                snprintf(st.message, sizeof(st.message), "%c key unsupported", c);
            }
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
                st.buf = realloc(st.buf, st.size + 1);
                if (!st.buf) {
                    snprintf(st.message, sizeof(st.message), "ENOMEM");
                    st.mode = COMMAND;
                    continue;
                }
                if (st.cursor < st.size)
                    memmove(st.buf + st.cursor + 1, st.buf + st.cursor, st.size - st.cursor);
                st.size += 1;
                st.buf[st.cursor++] = c;
                st.dirty = 1;
            }
        } else if (st.mode == PROMPT) {
            if (c == 27) {
                st.mode = COMMAND;
            } else if (c == 127) {
                if (strlen(st.prompt) > 0)
                    st.prompt[strlen(st.prompt) - 1] = 0;
            } else if (c == '\n') {
                for (char * s = st.prompt; *s; s++) {
                    if (*s == 'q') {
                        if (st.dirty && s[1] != '!') {
                            snprintf(st.message, sizeof(st.message), "dirty buffer");
                            st.mode = COMMAND;
                            continue;
                        }
                        goto quit;
                    } else if (*s == 'w') {
                        const char * path = (s[1] == ' ' && s[2]) ? s + 2 : NULL;
                        if (!st.orig_path[0] && !path) {
                            snprintf(st.message, sizeof(st.message), "no file name");
                            st.mode = COMMAND;
                            continue;
                        }
                        if (save_file(&st, path ? path : st.orig_path) < 0) {
                            st.mode = COMMAND;
                            continue;
                        }
                        if (path)
                            break;
                    }
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
