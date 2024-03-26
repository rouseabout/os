#include <errno.h>
#include <semaphore.h>
#include <syslog.h>

int sem_init(sem_t * sem, int pshared, unsigned value)
{
    syslog(LOG_DEBUG, "libc: sem_init");
    errno = ENOSYS;
    return -1;
}

int sem_post(sem_t * sem)
{
    syslog(LOG_DEBUG, "libc: sem_post");
    errno = ENOSYS;
    return -1;
}

int sem_wait(sem_t * sem)
{
    syslog(LOG_DEBUG, "libc: sem_wait");
    errno = ENOSYS;
    return -1;
}
