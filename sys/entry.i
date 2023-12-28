; system entrypoint

.org 0

.value SYS_MAJOR 0
.value SYS_MINOR 1

restart:
    mov sp, 0x8000      ; setup stack
    mov bp, 0x8000

    mov r0, 0x1000      ; default console device
    call iosel

    mov r0, SYS_MINOR
    push r0
    mov r0, SYS_MAJOR
    push r1
    mov r0, s_welcome
    call printf
    add sp, 4

    call sh

hang:
    jmp hang

s_welcome:
.string "sys impl %x.%x\n"
