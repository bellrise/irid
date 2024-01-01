; sh.i
; System shell app.
; Copyright (c) 2023-2024 bellrise

.export sh

.valuefile "arch.i"

sh:
@sh_loop:
    mov r0, s1      ; shell prompt
    call puts

    mov r0, inbuf   ; zero-out the input buffer
    mov r1, 256
    call bzero

    mov r0, inbuf   ; read line from user
    mov r1, 256
    call readline

    mov r0, inbuf   ; cmp with "lsdev"
    mov r1, s_lsdev
    call strcmp
    cmp r0, 0
    jeq @lsdev

    mov r0, inbuf   ; cmp with "echo"
    mov r1, s_echo
    call strcmp
    cmp r0, 0
    jeq @echo

    jmp @sh_loop    ; no command found

@lsdev:
    call lsdev
    jmp @sh_loop
@echo:
    mov r0, inbuf
    add r0, 5
    call puts
    mov r0, '\n'
    call putc
    jmp @sh_loop

    ret

; List devices.
; lsdev()
lsdev:
    push bp
    push r4
    push r5
    mov bp, sp

    sub sp, 64      ; device_ids = alloca u16[32]
    sub sp, 16      ; deviceinfo = alloca struct deviceinfo

    mov r0, s_lsdev_1
    call printf

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

    mov r0, 0x14    ; query device info
    load r1, r3     ; device ID
    mov r2, bp
    sub r2, 80      ; &deviceinfo
    cpucall

    mov r0, bp
    sub r0, 80      ; &deviceinfo
    add r0, 2       ; &deviceinfo->d_name
    push r0         ; printf name arg
    load r0, r3     ; device ID
    push r0         ; printf ID arg

    mov r0, s_lsdev_2
    call printf
    add sp, 4       ; remove arguments from stack

    add r5, 1
    jmp @loop

@end:
    mov sp, bp
    pop r5
    pop r4
    pop bp
    ret


; Read a line from the console. Returns the amount of characters read.
; readline(char *buf, int n) -> int
readline:
    push r4
    push r5
    push r6

    mov r4, r0      ; char *buf
    mov r5, r1      ; int n
    mov r6, 0       ; bytes read

@loop:
    call getc
    push r0
    call putc       ; echo the char back
    pop r0

    add r4, 1       ; increment index
    add r6, 1

    cmp r5, r6      ; check for end of buffer
    jeq @end

    cmp h0, '\n'    ; end of line
    jeq @end

    sub r4, 1       ; write the char to the buffer
    store h0, r4
    add r4, 1

    jmp @loop

@end:
    mov r0, r5
    pop r6
    pop r5
    pop r4
    ret


s1:
.string "# "
s2:
.string "`%s`"
s_echo:
.string "echo"
s_lsdev:
.string "lsdev"
s_lsdev_1:
.string "ID     NAME\n"
s_lsdev_2:
.string "0x%x %s\n"

inbuf:
.resv 256
