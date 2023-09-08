; Test script

.org 0x0000

restart:
    ; Set up stack at 0x8000
    mov sp, 0x8000
    mov bp, sp

    ; Set up keyboard interrupt routine
    mov r0, 0x15
    mov r1, 0x1000
    mov r2, keyboard_handler
    cpucall

    call query_devices

    sti             ; enable and wait for interrupts

@wait:
    call echo_input
    jmp @wait


; echo_input
; Echo the input buffer back to the user.
echo_input:
    ; Check if we can read something from the input buffer
    load h0, V_wptr
    load l0, V_rptr
    cmp h0, l0      ; compare the write & read pointers
    jeq @end        ; nothing to read

    mov r3, B_input
    add r3, l0          ; offset the pointer into the buffer

    load h2, r3         ; read the byte
    add l0, 1           ; increment the read pointer
    store l0, V_rptr    ; save the read pointer

    mov h0, h2
    call printchar

@end:
    ret

; query_devices
; Print all accesible devices.
query_devices:
    push sp
    push r4
    push r5
    push r6
    push r7

    sub sp, 128     ; alloca u16[64]

    mov r0, S_devices
    call print

    mov r0, 0x13    ; query devices
    mov r1, sp      ; u16[]
    mov r2, 64      ; max 64 elements
    cpucall

    mov r4, r2      ; counter
    mov r6, r1      ; device IDs

@next_device:
    load r5, r6     ; device ID
    sub sp, 16      ; alloca struct device_info
    mov r7, sp

    mov r0, 0x14    ; query device info
    mov r1, r5      ; device ID
    mov r2, r7      ; device info struct
    cpucall

    mov r0, S_deviceprefix
    call print

    mov r0, r5      ; print device ID
    call printhex

    mov h0, ' '
    call printchar

    add r7, 2       ; point into info->d_name[14]
    mov r0, r7
    call print      ; print d_name

    mov h0, '\n'    ; print newline
    call printchar

    add sp, 16      ; free device_info struct

    sub r4, 1       ; counter --
    add r6, 2       ; device IDs ++
    jnz r4, @next_device

    add sp, 128     ; free device ID list

    pop r7
    pop r6
    pop r5
    pop r4
    pop sp
    ret

; print
; Prints a null-terminated string.
; r0 - pointer to string
print:
    push sp
    push r4

    mov r4, r0      ; keep the string pointer

@loop:
    load h0, r4     ; load char into h2
    cmp h0, 0       ; check for null char
    jeq @end        ; stop printing

    call printchar  ; print the char

    add r4, 1       ; move to the next char
    jmp @loop       ; continue printing

@end:
    pop r4
    pop sp
    ret

; printchar
; Print a single ASCII char.
; h0 - the char to print
printchar:
    mov h2, h0      ; set the char to write
    mov r0, 0x20    ; write
    mov r1, 0x1000  ; terminal device
    cpucall         ; print char
    ret

; printhex
; Print a word (u16 integer) in hexadecimal format.
; r0 - word to print
printhex:
    push sp
    push r4
    push r5

    mov r4, r0          ; store original word
    mov r5, 4           ; loop 4 times

@loop:
    mov r6, r4          ; get the original number
    and r6, 0xf000      ; get the first nibble
    shr r6, 12          ; move 12 bits back

    add r6, '0'         ; byte -> ASCII
    cml r6, 0x3a        ; check if <= '9'
    jeq @dontadd

    add r6, 0x27        ; move to 'a'-'f' range

@dontadd:
    mov h0, r6
    call printchar      ; print the char

    shl r4, 4           ; move to the next nibble
    sub r5, 1           ; decrement loop
    jnz r5, @loop

    pop r5
    pop r4
    pop sp
    ret

; isalnum
; Check if the character is alphanumeric (in: /0-9A-z/).
; h0 - char to check
isalnum:
    ; '0' .. '9'
    cml h0, '0'
    jeq @false
    cml h0, ':'
    jeq @true

    ; 'A' .. 'Z'
    cml h0, 'A'
    jeq @false
    cml h0, 'Z'
    jeq @true

    ; 'a' .. 'z'
    cml h0, 'a'
    jeq @false
    cml h0, 'z'
    jeq @true

@true:
    mov r0, 1
    ret

@false:
    mov r0, 0
    ret

; strlen
; Calculate the length of a string.
; r0 - pointer to string
strlen:
    mov r1, r0
    mov r0, 0

@loop:
    load h2, r1     ; load char into h2
    cmp h2, 0       ; check for null char
    jeq @end        ; stop looping

    add r0, 1       ; increment counter
    add r1, 1       ; move pointer to the next char
    jmp @loop       ; continue looping

@end:
    ret

; strncmp
; Compare 2 strings.
; r0 - first string
; r1 - second string
; r2 - length to compare
strncmp:

@loop:
    cmp r2, 0       ; end of loop
    jeq @end

    load h3, r0     ; load the first char
    load l3, r1     ; load the second char
    cmp h3, l3      ; compare the 2 chars

    sub r2, 1       ; decrement the pointer
    add r0, 1       ; inc ptr1
    add r1, 1       ; inc ptr2

    jeq @loop       ; if the chars are the same, loop again
    mov r0, 0       ; otherwise, return false
    ret

@end:
    mov r0, 1       ; return true
    ret

keyboard_handler:
    ; Read a byte from the device
    mov r0, 0x21
    mov r1, 0x1000
    cpucall

    load h0, V_wptr ; load the write pointer
    mov r3, B_input
    add r3, h0      ; offset the pointer into the buffer
    add h0, 1       ; increment the write pointer
    store h0, V_wptr

    store h2, r3    ; write the char into the buffer

    rti             ; return from interrupt


S_hello:
.string "hello"

S_hi:
.string "hi"

S_devices:
.string "Devices:\n"

S_deviceprefix:
.string "  0x"

V_wptr:
.byte 0

V_rptr:
.byte 0

B_input:
.res 256
