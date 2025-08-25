#include <termios.h>
#include <os/syscall.h>
#include <syslog.h>
#include <sys/ioctl.h>

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

int tcgetattr(int fildes, struct termios *termios_p)
{
    return ioctl(fildes, TCGETS, termios_p);
}

int tcsendbreak(int fildes, int duration)
{
    syslog(LOG_DEBUG, "libc: tcsendbreak");
    return 0; //FIXME:
}

int tcsetattr(int fildes, int optional_actions, const struct termios *termios_p)
{
    int opt;
    switch (optional_actions) {
    case TCSANOW: opt = TCSETS; break;
    case TCSADRAIN: opt = TCSETSW; break;
    case TCSAFLUSH: opt = TCSETSF; break;
    default:
        errno = ENOSYS;
        return -1;
    }
    return ioctl(fildes, opt, termios_p);
}
