#include <termios.h>
#include <os/syscall.h>
#include <syslog.h>

speed_t cfgetispeed(const struct termios * termios_p)
{
    return 0; //FIXME:
}

speed_t cfgetospeed(const struct termios * termios_p)
{
    return 0; //FIXME:
}

int cfsetispeed(struct termios *termios_p, speed_t speed)
{
    return 0; //FIXME:
}

int cfsetospeed(struct termios * termios_p, speed_t speed)
{
    return 0; //FIXME:
}

int tcdrain(int fildes)
{
    syslog(LOG_DEBUG, "libc: tcdrain");
    return 0; //FIXME:
}

int tcflow(int fildes, int action)
{
    syslog(LOG_DEBUG, "libc: tcflow");
    return 0; //FIXME:
}

int tcflush(int fildes, int queue_selector)
{
    syslog(LOG_DEBUG, "libc: tcflush");
    return 0; //FIXME:
}

MK_SYSCALL2(int, tcgetattr, OS_TCGETATTR, int, struct termios *)

int tcsendbreak(int fildes, int duration)
{
    syslog(LOG_DEBUG, "libc: tcsendbreak");
    return 0; //FIXME:
}

MK_SYSCALL3(int, tcsetattr, OS_TCSETATTR, int, int, const struct termios *)
