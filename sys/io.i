; io.i
; Library for input/output routines.
; Copyright (c) 2023 bellrise

.export putc
.export puts
.export putx
.export getc
.export printf
.export iosel

.valuefile "arch.i"


; Write a character to the selected I/O device.
; putc(char c)
putc:
    mov h2, r0
    mov r0, CPUCALL_DEVICEWRITE
    load r1, __io_dev
    cpucall
    ret

; Write a string to the selected I/O device.
; puts(char *str)
puts:
    push bp
    push r4
    mov bp, sp

    mov r4, r0      ; char *str

@loop:
    load h0, r4     ; load char
    cmp h0, 0       ; check if string end
    jeq @end
    call putc       ; print char
    add r4, 1       ; move to next char
    jmp @loop

@end:
    pop r4
    pop bp
    ret

; Print a hex number.
; putx(int)
putx:
    push bp
    push r4
    push r5
    mov bp, sp

    mov r4, r0      ; int num
    mov r5, 4       ; counter

@loop:
    cmp r5, 0
    jeq @end

    mov r0, r4
    and r0, 0xf000
    shr r0, 12

    add r0, '0'
    cmg r0, '9'     ; if char > '9'
    jeq @over_9
    jmp @print

@over_9:
    add r0, 7       ; move to 'A-F'

@print:
    call putc

    shl r4, 4
    sub r5, 1
    jmp @loop

@end:
    pop r5
    pop r4
    pop bp
    ret

; Read a single character.
; getc()
getc:
    push bp

    mov r0, CPUCALL_DEVICEPOLL
@wait:
    load r1, __io_dev
    cpucall
    cmp h2, 0       ; read queue empty
    jeq @wait

    mov r0, CPUCALL_DEVICEREAD
    load r1, __io_dev
    cpucall
    mov r0, h2      ; read char from device

    pop bp
    ret

; Print a formatted string.
; printf(char *fmt, ...)
printf:
    push bp
    push r4
    push r5
    mov bp, sp

    mov r4, r0      ; char *fmt
    mov r5, bp      ; &...
    add r5, 8

@loop:
    load h0, r4     ; load char
    cmp h0, 0       ; check for end of string
    jeq @end

    cmp h0, '%'     ; compare char
    jeq @format

    call putc       ; print non-formatted char
    add r4, 1       ; increment fmt pointer
    jmp @loop

@format:
    add r4, 1       ; move to next char
    load h0, r4     ; load char

    cmp h0, '%'     ; regular percent
    jeq @LF0
    cmp h0, 'x'     ; hex number
    jeq @LFx
    cmp h0, 's'     ; string
    jeq @LFs

    push r0         ; print unknown %_ otherwise
    mov r0, '%'
    call putc
    pop r0
    call putc
    jmp @LFend

@LF0:
    mov h0, '%'
    call putc
    jmp @LFend

@LFx:
    load r0, r5     ; get argument from stack
    add r5, 2       ; move to next arg
    call putx
    jmp @LFend

@LFs:
    load r0, r5     ; char * from stack
    add r5, 2       ; move to next arg
    call puts
    jmp @LFend

@LFend:
    add r4, 1       ; move to next char
    jmp @loop

@end:
    mov sp, bp
    pop r5
    pop r4
    pop bp
    ret

; Select the I/O device to use.
; iosel(int dev)
iosel:
    store r0, __io_dev
    ret

__io_dev:
.resv 2
