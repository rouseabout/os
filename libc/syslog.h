#ifndef SYSLOG_H
#define SYSLOG_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    LOG_EMERG = 0,
    LOG_ALERT,
    LOG_CRIT,
    LOG_ERR,
    LOG_WARNING,
    LOG_NOTICE,
    LOG_INFO,
    LOG_DEBUG
};

enum {
    LOG_DAEMON = 0,
    LOG_AUTH,
    LOG_CRON,
    LOG_KERN,
    LOG_LOCAL0,
    LOG_LOCAL1,
    LOG_LOCAL2,
    LOG_LOCAL3,
    LOG_LOCAL4,
    LOG_LOCAL5,
    LOG_LOCAL6,
    LOG_LOCAL7,
    LOG_LPR,
    LOG_MAIL,
    LOG_NEWS,
    LOG_SYSLOG,
    LOG_USER,
    LOG_UUCP
};

#define LOG_PID 0
#define LOG_CONS 0
#define LOG_NDELAY 0

#define LOG_MASK(pri) (1 << (pri))

void closelog(void);
#define openlog(a, b, c) do { } while(0)
int setlogmask(int);
void syslog(int, const char *, ...);

#ifdef __cplusplus
}
#endif

#endif /* SYSLOG_H */
