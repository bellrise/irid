; io.i
; Library for input/output routines.

.export putc
.export puts
.export putx
.export getc
.export io_sel
.export io_devd

.value CPUCALL_DEVICEWRITE 0x20
.value CPUCALL_DEVICEREAD  0x21
.value CPUCALL_DEVICEPOLL  0x22


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
    mov r0, '\n'
    call putc

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

; Select the I/O device to use.
; io_sel(int dev)
io_sel:
    store r0, __io_dev
    ret

; Dump all connected devices.
; io_devd()
io_devd:
    push sp
    push bp
    push r4
    push r5
    mov bp, sp

    sub sp, 64      ; device_ids = alloca u16[32]
    sub sp, 16      ; deviceinfo = alloca struct deviceinfo

    mov r0, 0x13    ; query device IDs
    mov r1, bp
    sub r1, 64      ; &device_ids
    mov r2, 32      ; device_ids count
    cpucall

    mov r4, r2      ; amount of devices
    mov r5, 0       ; index

@loop:
    cmp r5, r4      ; if index == n devices
    jeq @end

    mov r3, r5
    mul r3, 2       ; offset into device ID array
    mov r0, bp
    sub r0, 64      ; &device_ids
    add r3, r0      ; &device_ids[index]

    load r0, r3
    call putx       ; print device ID
    mov r0, ':'
    call putc

    mov r0, 0x14    ; query device info
    load r1, r3     ; device ID
    mov r2, bp
    sub r2, 80      ; &deviceinfo
    cpucall

    mov r0, bp
    sub r0, 80      ; &deviceinfo
    add r0, 2       ; &deviceinfo->d_name
    call puts

    add r5, 1
    jmp @loop

@end:
    mov sp, bp
    pop r5
    pop r4
    pop bp
    pop sp
    ret

__io_dev:
.resv 2
