; string.i
; Library for string operations.
; Copyright (c) 2023-2024 bellrise

.export strlen
.export strcmp

; Calculate the length of a string.
; strlen(char *s) -> int
strlen:
    mov r1, r0
    mov r0, 0

@loop:
    load h2, r1     ; load char
    cmp h2, 0       ; check for null terminator
    jeq @end

    add r0, 1       ; increment counter
    add r1, 1       ; increment string
    jmp @loop

@end:
    ret


; Compare 2 strings. Returns 0 in r0 for equal strings.
; strcmp(char *first, char *second) -> int
strcmp:
    push r0
    push r1

    call strlen     ; strlen(first)
    mov r2, r0
    pop r0
    push r0
    push r2
    call strlen     ; strlen(second)
    mov r3, r0
    pop r2

    cmp r2, r3      ; len first == len second
    pop r3          ; first
    pop r2          ; second
    jeq @loop
    jmp @not_equal

@loop:
    load h1, r2     ; fc = first[i]
    load l1, r3     ; sc = second[i]

    cmp h1, 0       ; check for end of string
    jeq @equal

    cmp h1, l1      ; fc == sc
    jeq @L1
    jmp @not_equal

@L1:
    add r2, 1       ; first++
    add r3, 1       ; second++
    jmp @loop

@not_equal:
    mov r0, 1
    ret

@equal:
    mov r0, 0
    ret
