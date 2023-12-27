; system entrypoint

.org 0

__start:
    mov sp, 0x8000          ; setup stack
    mov bp, 0x8000

    mov r0, 0x1000          ; use the console
    call io_sel

sys:
    call getc
    mov r4, r0

    mov r0, r4
    call putc
    jmp sys

hang:
    jmp hang

s_readchar:
.string "Read char:"
