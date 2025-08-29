bits 64
section .text

%define OS_CLONE 56
%define OS_EXIT 60

global do_clone
do_clone:
    push r8
    push r10

    mov [rsi - 8], r8 ; value_ptr
    mov [rsi - 16], rcx ; args
    mov [rsi - 24], rdx ; start_routine
    sub rsi, 24

    mov r8, 0 ; ; child_tidptr
    mov r10, 0 ; tls
    mov rdx, 0 ; parent_tidptr
    mov rax, OS_CLONE
    syscall

    test rax, rax
    jz .child

    pop r10
    pop r8
    ret

.child:
    pop rsi
    pop rdi
    call rsi

    pop rsi
    mov [rsi], rax

    mov rdi, 0
    mov rax, OS_EXIT
    syscall
