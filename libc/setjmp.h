#ifndef SETJMP_H
#define SETJMP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { int reg[6]; } jmp_buf[1];
typedef struct { int reg[6]; } sigjmp_buf[1];

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
