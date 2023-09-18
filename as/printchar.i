
.export printchar
; Print a character.
; h0 - char to print
printchar:
    mov h2, h0
    mov r0, 0x20
    mov r1, 0x1000
    cpucall
    ret
