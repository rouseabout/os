#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int foo;
} sem_t;

int sem_init(sem_t *, int, unsigned);
int sem_post(sem_t *);
int sem_wait(sem_t *);

#ifdef __cplusplus
}
#endif

#endif /* SEMAPHORE_H */
