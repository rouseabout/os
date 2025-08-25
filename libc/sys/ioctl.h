#ifndef SYS_IOCTL_H
#define SYS_IOCTL_H

#ifdef __cplusplus
extern "C" {
#endif

#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404
#define TIOCGPGRP 0x540F
#define TIOCSPGRP 0x5410
#define TIOCGWINSZ 0x5413
#define TIOCSWINSZ 0x5414
#define FIONBIO 0x5421

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

int ioctl(int, int, ...);

#ifdef __cplusplus
}
#endif

#endif /* SYS_IOCTL */
