#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/wait.h>
#include <os/syscall.h>

int pthread_attr_init(pthread_attr_t * attr)
{
    return 0; ///FIXME:
}

int pthread_attr_setdetachstate(pthread_attr_t * attr, int detachstate)
{
    return 0; //FIXME:
}

int pthread_cond_broadcast(pthread_cond_t * cond)
{
    return 0; //FIXME:
}

int pthread_cond_destroy(pthread_cond_t * cond)
{
    return 0; //FIXME:
}

int pthread_cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr)
{
    return 0; //FIXME:
}

int pthread_cond_signal(pthread_cond_t * cond)
{
    return 0; //FIXME:
}

int pthread_cond_timedwait(pthread_cond_t * cond, pthread_mutex_t * mutex, const struct timespec * abstime)
{
    return 0; //FIXME:
}

int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex)
{
    return 0; //FIXME:
}

struct pthread {
    char * stack;
    int parent_tid;
    int child_tid;
    void * value;
};

int do_clone(int clone_flags, void * stack, void * (*start_routine)(void*), void * arg, void ** value_ptr);

int pthread_create(pthread_t * thread, const pthread_attr_t * attr, void *(*start_routine)(void*), void * arg)
{
    struct pthread * pd = malloc(sizeof(struct pthread));
    if (!pd) {
        errno = EAGAIN;
        return -1;
    }

    int stack_size = 0x1000;

    pd->stack = malloc(stack_size);
    if (!pd->stack) {
        free(pd);
        errno = EAGAIN;
        return -1;
    }

    int pid = do_clone(CLONE_VM | CLONE_FILES | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | /*CLONE_THREAD | CLONE_SETTLS |*/ SIGCHLD, pd->stack + stack_size, start_routine, arg, &pd->value);
    if (pid < 0) {
        errno = pid;
        return -1;
    }

    pd->child_tid = pid;
    *thread = (unsigned long)pd;
    return 0;
}

int pthread_detach(pthread_t thread)
{
    return 0; //FIXME:
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return t1 == t2;
}

void pthread_exit(void *value_ptr)
{
    syslog(LOG_DEBUG, "libc: pthread_exit");
}

void * pthread_getspecific(pthread_key_t key)
{
    syslog(LOG_DEBUG, "libc: pthread_getspecific");
    return NULL; //FIXME:
}

int pthread_join(pthread_t thread, void ** value_ptr)
{
    struct pthread * pd = (struct pthread *)thread;
    waitpid(pd->child_tid, NULL, 0);
    *value_ptr = pd->value;
    free(pd->stack);
    free(pd);
    return 0;
}

int pthread_key_create(pthread_key_t * key, void (*destructor)(void*))
{
    syslog(LOG_DEBUG, "libc: pthread_key_create");
    return 0; //FIXME:
}

int pthread_key_delete(pthread_key_t key)
{
    syslog(LOG_DEBUG, "libc: pthread_key_delete");
    return 0; //FIXME:
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    return 0; //FIXME:
}

int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t * attr)
{
    return 0; //FIXME:
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    return 0; //FIXME:
}

int pthread_mutexattr_destroy(pthread_mutexattr_t * attr)
{
    return 0; //FIXME:
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t * attr, int * type)
{
    return 0; //FIXME:
}

int pthread_mutexattr_init(pthread_mutexattr_t * attr)
{
    return 0; //FIXME:
}

int pthread_mutexattr_settype(pthread_mutexattr_t * attr, int type)
{
    return 0; //FIXME:
}

pthread_t pthread_self(void)
{
    return 0; //FIXME:
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    syslog(LOG_DEBUG, "libc: pthread_setspecific");
    return 0; //FIXME:
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    return 0; //FIXME:
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    return 0; //FIXME:
}

int pthread_once(pthread_once_t * once_control, void (*init_routine)(void))
{
    return 0; //FIXME:
}

int pthread_setcancelstate(int state, int * oldstate)
{
    syslog(LOG_DEBUG, "libc: pthread_setcancelstate");
    return 0;
}
