#ifndef PTHREAD_H
#define PTHREAD_H

#include <sys/types.h>
#include <sched.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PTHREAD_CANCEL_DISABLE 0

#define PTHREAD_CREATE_DETACHED 0
#define PTHREAD_CREATE_JOINABLE 0

#define PTHREAD_MUTEX_INITIALIZER 0
#define PTHREAD_ONCE_INIT 0
#define PTHREAD_MUTEX_RECURSIVE 0

int pthread_attr_init(pthread_attr_t *);
int pthread_attr_setdetachstate(pthread_attr_t *, int);
int pthread_cancel(pthread_t);
int pthread_cond_broadcast(pthread_cond_t *);
int pthread_cond_destroy(pthread_cond_t *);
int pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
int pthread_cond_signal(pthread_cond_t *);
int pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *);
int pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void*), void *);
int pthread_detach(pthread_t);
int pthread_equal(pthread_t, pthread_t);
void pthread_exit(void *);
void * pthread_getspecific(pthread_key_t);
int pthread_join(pthread_t, void **);
int pthread_key_create(pthread_key_t *, void (*)(void*));
int pthread_key_delete(pthread_key_t);
int pthread_mutex_destroy(pthread_mutex_t *);
int pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_trylock(pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *);
int pthread_mutexattr_destroy(pthread_mutexattr_t *);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *, int *);
int pthread_mutexattr_init(pthread_mutexattr_t *);
int pthread_mutexattr_settype(pthread_mutexattr_t *, int);
int pthread_once(pthread_once_t *, void (*)(void));
pthread_t pthread_self(void);
int pthread_setcancelstate(int, int *);
int pthread_setspecific(pthread_key_t, const void *);

#ifdef __cplusplus
}
#endif

#endif /* PTHREAD_H */
