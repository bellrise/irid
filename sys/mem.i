; mem.i
; Library for in-memory operations.
; Copyright (c) 2023 bellrise

.export bzero

; Set a range of memory to all zeroes.
; bzero(char *, int n)
bzero:
    mov h2, 0
@remove_byte:
    cmp r1, 0           ; end of buffer
    jeq @end
    store h2, r0        ; write a 0
    add r0, 1           ; increment pointer
    sub r1, 1           ; decrement counter
    jmp @remove_byte
@end:
    ret
