bits 32
section .text

global setjmp
global _setjmp
global sigsetjmp
setjmp:
_setjmp:
sigsetjmp:
    mov ecx, [esp+4] ; jmpbuf
    mov edx, [esp+0] ; return address

    mov [ecx+0], ebx
    mov [ecx+4], esi
    mov [ecx+8], edi
    mov [ecx+12], ebp
    mov [ecx+16], esp
    mov [ecx+20], edx

    xor eax, eax
    ret

global longjmp
global _longjmp
global siglongjmp
longjmp:
_longjmp:
siglongjmp:
    mov ecx, [esp+4] ; jmpbuf
    mov eax, [esp+8] ; value

    test eax, eax
    jnz .skip
    mov eax, 1
.skip:

    mov ebx, [ecx+0]
    mov esi, [ecx+4]
    mov edi, [ecx+8]
    mov ebp, [ecx+12]
    mov esp, [ecx+16]
    mov edx, [ecx+20]
    mov [esp], edx ; return address

    ret
