#ifndef ARPA_INET_H
#define ARPA_INET_H

#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t htonl(uint32_t);
uint16_t htons(uint16_t);
in_addr_t inet_addr(const char *);
char * inet_ntoa(struct in_addr);
const char * inet_ntop(int, const void *, char *, socklen_t);
int inet_pton(int, const char *, void *);
uint32_t ntohl(uint32_t);
uint16_t ntohs(uint16_t);

#ifdef __cplusplus
}
#endif

#endif /* ARPA_INET_H */
