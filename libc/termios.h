#ifndef TERMIOS_H
#define TERMIOS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef char cc_t;
typedef int speed_t;
typedef int tcflag_t;

#define NCCS 32

#define VEOF 0
#define VEOL 0
#define VERASE 0
#define VINTR 0
#define VKILL 0
#define VMIN 0
#define VTIME 0
#define VSTART 0
#define VSTOP 0
#define VSUSP 0
#define VQUIT 0
#define VLNEXT 0
#define VEOL2 0

// input modes

#define BRKINT 0x1
#define ICRNL 0x2
#define IGNBRK 0x4
#define IGNCR 0x8
#define IGNPAR 0x10
#define INLCR 0x20
#define INPCK 0x40
#define ISTRIP 0x80
#define IXANY 0x100
#define IXOFF 0x200
#define IXON 0x400
#define PARMRK 0x800
#define IMAXBEL 0x1000

// output modes

#define OPOST 0x1
#define ONLCR 0x2
#define OCRNL 0x4
#define ONOCR 0x10
#define ONLRET 0x20
#define OFDEL 0x40
#define OFILL 0x80
#define NLDLY 0x100
#define NL0 0x0
#define NL1 0x100

// baud rate selection

#define B0 0
#define B50 50
#define B75 75
#define B110 110
#define B134 134 /* 134.5 */
#define B150 150
#define B200 200
#define B300 300
#define B600 600
#define B1200 1200
#define B1800 1800
#define B2400 2400
#define B4800 4800
#define B9600 9600
#define B19200 19200
#define B38400 38400

// control modes

#define CSIZE 0
#define CS5 5
#define CS6 6
#define CS7 7
#define CS8 8
#define CSTOPB 0
#define CREAD 0
#define PARENB 0
#define PARODD 0
#define HUPCL 0
#define CLOCAL 0

// local modes

#define ECHO 8
#define ECHOE 0
#define ECHOK 0
#define ECHONL 0
#define ICANON 2
#define IEXTEN 0
#define ISIG 1
#define NOFLSH 0
#define TOSTOP 0
#define FLUSHO 0

#define ECHOCTL 0
#define ECHOKE 0

// Attribute Selection

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

// Line Control

#define TCIFLUSH 1
#define TCOFLUSH 2
#define TCIOFLUSH 3

#define TCIOFF 1
#define TCION 2
#define TCOOFF 3
#define TCOON 4

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[NCCS];
};

speed_t cfgetispeed(const struct termios *);
speed_t cfgetospeed(const struct termios *);
int cfsetispeed(struct termios *, speed_t);
int cfsetospeed(struct termios *, speed_t);
int tcdrain(int);
int tcflow(int, int);
int tcflush(int, int);
int tcgetattr(int, struct termios *);
int tcsendbreak(int, int);
int tcsetattr(int, int, const struct termios *);

#ifdef __cplusplus
}
#endif

#endif /* TERMIOS_H */
