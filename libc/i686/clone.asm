bits 32
section .text

%define OS_EXIT 1
%define OS_CLONE 120

global do_clone
do_clone:
    push ebp
    mov ebp, esp

    push ebx
    push esi
    push edi

    mov edi, [ebp+24] ; value_ptr
    mov esi, [ebp+20] ; arg
    mov edx, [ebp+16] ; start_routine
    mov ecx, [ebp+12] ; newsp
    mov ebx, [ebp+8] ; clone_flags

    mov [ecx - 4], edi
    mov [ecx - 8], esi
    mov [ecx - 12], edx
    sub ecx, 12

    mov edi, 0 ; ; child_tidptr
    mov esi, 0 ; tls
    mov edx, 0 ; parent_tidptr
    mov eax, OS_CLONE
    int 0x80

    test eax, eax
    jz .child

    pop edi
    pop esi
    pop ebx

    pop ebp
    ret

.child:
    pop edx
    call edx
    add esp, 4

    pop edx
    mov dword [edx], eax

    mov ebx, 0
    mov eax, OS_EXIT
    int 0x80
