bits 32
section .text

%macro DEFISR 1
global isr%1
isr%1:
    cli
    push 0
    push %1
    jmp isr_common
%endmacro

%macro DEFISR_ERRORCODE 1
global isr%1
isr%1:
    cli
    push %1
    jmp isr_common
%endmacro

%macro DEFIRQ 1
global irq%1
irq%1:
    cli
    push 0
    push 32 + %1
    jmp isr_common
%endmacro

%if 1
DEFISR 0
DEFISR 1
DEFISR 2
DEFISR 3
DEFISR 4
DEFISR 5
DEFISR 6
DEFISR 7
DEFISR_ERRORCODE 8
DEFISR 9
DEFISR_ERRORCODE 10
DEFISR_ERRORCODE 11
DEFISR_ERRORCODE 12
DEFISR_ERRORCODE 13
DEFISR_ERRORCODE 14
DEFISR 15
DEFISR 16
DEFISR_ERRORCODE 17
DEFISR 18
DEFISR 19
DEFISR 20
DEFISR_ERRORCODE 21 ;;??errorcode
DEFISR 22
DEFISR 23
DEFISR 24
DEFISR 25
DEFISR 26
DEFISR 27
DEFISR 28
DEFISR 29
DEFISR 30  ;; ??errorcode
DEFISR 31

DEFIRQ 0
DEFIRQ 1
DEFIRQ 2
DEFIRQ 3
DEFIRQ 4
DEFIRQ 5
DEFIRQ 6
DEFIRQ 7
DEFIRQ 8
DEFIRQ 9
DEFIRQ 10
DEFIRQ 11
DEFIRQ 12
DEFIRQ 13
DEFIRQ 14
DEFIRQ 15

DEFISR 128
%endif

extern interrupt_handler

isr_common:
    cld

    test word [esp + 12], 0x3 ; cs
    jz .ring0_entry
.entry_continue:
    push eax
    push ecx
    push edx
    push ebx
    push ebp
    push esi
    push edi

    mov ebp, ds   ; ds
    push ebp

    mov bp, 0x10  ; use ring0 data segment
    mov ds, bp
    mov es, bp
    mov fs, bp
    mov gs, bp

    mov ebx, esp
    sub esp, 4
    and esp, ~0xF  ; align stack
    mov [esp], ebx

    xor ebp, ebp
    call interrupt_handler
    mov esp, ebx

    pop ebp       ; ds
    mov ds, bp
    mov es, bp
    mov fs, bp
    mov gs, bp

    pop edi
    pop esi
    pop ebp
    pop ebx
    pop edx
    pop ecx
    pop eax

    add esp, 8

    test word [esp + 4], 0x3  ; cs
    jz .ring0_exit
.exit_continue:
    iret

    ; when ring0 is interrupted, stack segment and stack pointer are not pushed to stack
.ring0_entry:
    mov [esp -  4 - 8], eax  ; save eax

    mov eax, [esp + 0]      ; interrupt number
    mov [esp +  0 - 8], eax

    mov eax, [esp + 4]      ; error code
    mov [esp +  4 - 8], eax

    mov eax, [esp + 8]      ; eip
    mov [esp +  8 - 8], eax

    mov eax, [esp + 12]     ; cs
    mov [esp + 12 - 8], eax

    mov eax, [esp + 16]     ; eflags
    mov [esp + 16 - 8], eax

    mov eax, esp            ; esp
    add eax, 5*4
    mov [esp + 20 - 8], eax

    mov eax, ss             ; ss
    mov [esp + 24 - 8],eax

    mov eax,[esp - 4 - 8]   ; restore eax

    sub esp, 8
    jmp .entry_continue

    ; when ring0 is interrupted, iret does dont pop stack segment and stack pointer from stack
.ring0_exit:
    mov [esp - 4], eax   ; save eax
    mov [esp - 8], ebx   ; save ebx
    mov [esp - 12], ecx  ; save ecx

    mov ebx, [esp + 12] ; esp
    sub ebx, 3 * 4

    mov cx, [esp + 16]  ; ss

    mov eax, [esp - 4]  ; interrupted eax
    mov [ebx - 12], eax

    mov eax, [esp - 8]  ; interrupted ebx
    mov [ebx - 16], eax

    mov eax, [esp - 12] ; interrupted ecx
    mov [ebx - 20], eax

    mov eax, [esp + 8]   ; eflags
    mov [ebx + 8], eax

    mov eax, [esp + 4]   ; cs
    mov [ebx + 4], eax

    mov eax, [esp + 0]   ; eip
    mov [ebx + 0], eax

    mov ss, ecx          ; load ss
    mov esp, ebx         ; load esp

    mov eax, [esp - 12]  ; restore eax
    mov ebx, [esp - 16]  ; restore ebx
    mov ecx, [esp - 20]  ; restore ecx
    jmp .exit_continue


global jmp_to_userspace
jmp_to_userspace:

    mov ebp, esp

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23     ; ss

    mov eax, [ebp + 8]
    push eax      ; userspace esp

    pushf         ; eflags

    push 0x1b     ; userspace cs

    mov eax, [ebp + 4]
    push eax      ; userspace eip

    xor ebp, ebp
    iret
