bits 64
section .text

global setjmp
global _setjmp
global sigsetjmp
setjmp:
_setjmp:
sigsetjmp:
    mov rax, [rsp+0] ; return address
    mov [rdi+0], rbx
    mov [rdi+8], rsp
    mov [rdi+16], rbp
    mov [rdi+24], r12
    mov [rdi+32], r13
    mov [rdi+40], r14
    mov [rdi+48], r15
    mov [rdi+56], rax
    xor rax, rax
    ret

global longjmp
global _longjmp
global siglongjmp
longjmp:
_longjmp:
siglongjmp:
    test rsi, rsi ; value
    jnz .skip
    mov rsi, 1
.skip:
    mov rbx, [rdi+0]
    mov rsp, [rdi+8]
    mov rbp, [rdi+16]
    mov r12, [rdi+24]
    mov r13, [rdi+32]
    mov r14, [rdi+40]
    mov r15, [rdi+48]
    mov rax, [rdi+56]
    mov [rsp], rax ; return address
    mov rax, rsi
    ret
