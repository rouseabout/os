#ifndef NET_IF_H
#define NET_IF_H

struct ifreq { /* not posix */
    char ifr_name[32];
};

unsigned if_nametoindex(const char *);

#endif /* NET_IF_H */
