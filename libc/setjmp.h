#ifndef SETJMP_H
#define SETJMP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int jmp_buf[6];
typedef int sigjmp_buf[6];

void longjmp(jmp_buf, int);
int setjmp(jmp_buf);
void _longjmp(jmp_buf, int);
int _setjmp(jmp_buf);
void siglongjmp(sigjmp_buf, int);
int sigsetjmp(sigjmp_buf, int);

#ifdef __cplusplus
}
#endif

#endif /* SETJMP_H */
