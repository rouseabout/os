#ifndef SYS_IOCTL_H
#define SYS_IOCTL_H

#ifdef __cplusplus
extern "C" {
#endif

#define FIONBIO 1
#define TIOCGWINSZ 100
#define TIOCSWINSZ 101

struct winsize {
    int ws_row;
    int ws_col;
    int ws_xpixel;
    int ws_ypixel;
};

int ioctl(int, int, ...);

#ifdef __cplusplus
}
#endif

#endif /* SYS_IOCTL */
