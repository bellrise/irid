; Test script

.org 0x0000

.value CONSOLE 0x1000
.value CCTL_MODE 0x11
.value CCTL_SIZE 0x01

restart:
    ; Set up stack at 0x8000
    mov sp, 0x8000
    mov bp, sp

    mov h0, 0x0c    ; clear screen
    call printchar

    call query_devices
    call query_console_size

@input:
    mov r0, S_prefix
    call print      ; print a prefix

    mov r0, B_input
    mov r1, 256
    call getline    ; get a line from the user

    mov r0, B_input
    call strlen     ; check the len of the input line

    ; lsdev
    mov r1, 6       ; 6 bytes for "lsdev ", r0 already has the strlen
    call min
    mov r2, r0      ; max len is the shorter string
    mov r0, B_input
    mov r1, S_lsdev
    call strncmp    ; check for "lsdev "
    jnz r0, @call_lsdev

    ; echo
    mov r1, 5
    call min
    mov r2, r0
    mov r0, B_input
    mov r1, S_echo
    call strncmp    ; check for "echo "
    jnz r0, @call_echo

    jmp @input

@call_lsdev:
    call query_devices
    jmp @input

@call_echo:
    mov r0, B_input
    add r0, 5       ; skip "echo "
    call print
    jmp @input

@wait:
    jmp @wait

; query_console_size
; Print the current console size.
query_console_size:
    push r4
    push r5

    mov h0, 0x11    ; control mode
    call printchar

    mov h0, 0x01    ; ask for size
    call printchar

    call getword    ; width
    mov r4, r0
    call getword    ; height
    mov r5, r0

    mov r0, S_console_size
    call print

    mov r0, r4      ; width
    call printint
    mov h0, 'x'
    call printchar
    mov r0, r5      ; height
    call printint

    mov h0, '\n'
    call printchar

    pop r5
    pop r4
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
; Write a byte to the console device.
; h0 - the char to print
printchar:
    mov h2, h0      ; set the char to write
    mov r0, 0x20    ; write
    mov r1, CONSOLE ; console device
    cpucall         ; print char
    ret

; getchar
; Get a single byte from the console device. Returns in h0. This function
; will block until there is something to read.
getchar:
    mov r0, 0x22    ; poll the device
    mov r1, CONSOLE
    cpucall

    cmp h2, 0       ; no data to read
    jeq getchar     ; try again

    mov r0, 0x21    ; read
    mov r1, CONSOLE ; console device
    cpucall
    mov h0, h2      ; move the read byte into the return register
    ret

; getline
; Get a full line from the console device. This function will block until
; the device provides a full line. Will set the last byte of the buffer
; to a NULL byte.
; r0 - u8[] buffer
; r1 - buffer len
getline:
    push r4
    push r5

    mov r4, r0      ; ptr
    mov r5, r1      ; len

@loop:
    call getchar
    store h0, r4    ; *ptr = byte

    push h0
    call printchar  ; echo it back

    add r4, 1       ; ptr++
    sub r5, 1       ; len--

    cmp r5, 1       ; check if we run out of buffer space
    jeq @endnull

    pop h0
    cmp h0, '\n'    ; see if this is the line end
    jeq @endline    ; still end it with a NUL byte (replacing the \n)

    jmp @loop

@endline:
    sub r4, 1

@endnull:
    mov h0, 0
    store h0, r4    ; place the NUL byte

    pop r5
    pop r4
    ret

; getword
; Get a whole word from the console device. Returns in r0.
getword:
    mov r0, 0x21    ; read
    mov r1, CONSOLE ; console device

    cpucall         ; high byte
    mov h3, h2
    cpucall         ; low byte
    mov l3, h2

    mov r0, r3
    ret

; printhex
; Print a word (u16 integer) in hexadecimal format.
; r0 - word to print
printhex:
    push r4
    push r5
    push r6

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

    pop r6
    pop r5
    pop r4
    ret

; mod
; Make a modulo operation. The result is stored back in r0.
; r0 - dividend
; r1 - divisor
mod:
    cml r0, r1      ; check if r0 < r1
    jeq @end

    sub r0, r1
    jmp mod

@end:
    ret

; min
; Returns the smaller number. The result is stored back in r0.
; r0 - first word
; r1 - second word
min:
    cmg r0, r1
    jeq @swap
    ret

@swap:
    mov r0, r1
    ret

; div
; Make a division opetaion. The result is stored back in r0.
; r0 - dividend
; r1 - divisor
div:
    mov r2, 0       ; amount of times the divisor fit

@loop:
    cml r0, r1      ; check if r0 < r1
    jeq @end

    sub r0, r1
    add r2, 1
    jmp @loop

@end:
    mov r0, r2
    ret

; printint
; Print a word (u16 integer) in decimal format.
; r0 - word to print
printint:
    push r4
    push r5

    mov r5, r0

    ; zero-out output buffer
    mov r0, B_printint
    mov h1, 0
    mov r2, 6
    call memset

    ; move pointer to end of buffer
    mov r4, B_printint
    add r4, 5

@loop:
    ; value % 10
    mov r0, r5
    mov r1, 10
    call mod

    add h0, '0'     ; move char into '0'-'9'
    store h0, r4    ; store char into buffer
    sub r4, 1       ; ptr--

    ; stop loop if value < 10
    cml r5, 10
    jeq @end

    ; value /= 10
    mov r0, r5
    mov r1, 10
    call div
    mov r5, r0

    jmp @loop

@end:
    mov r0, r4
    add r0, 1
    call print

    pop r5
    pop r4
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

; memset
; Fill a range of memory with a single byte.
; r0 - pointer to memory
; h1 - byte to write
; r2 - length of the range
memset:
    store h1, r0    ; write the byte
    add r0, 1       ; ptr++
    sub r2, 1       ; len--
    jnz r2, memset
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

S_devices:
.string "Devices:\n"

S_deviceprefix:
.string "  0x"

S_console_size:
.string "Console size: "

S_prefix:
.string "> "

S_lsdev:
.string "lsdev "

S_echo:
.string "echo "

B_input:
.resv 256

B_printint:
.resv 6
