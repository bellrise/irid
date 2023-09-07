; Test script

.org 0x0000


restart:
    ; Set up keyboard interrupt routine
    mov r0, 0x15
    mov r1, 0x1000
    mov r2, keyboard_handler
    cpucall

    ; Enable & wait for interrupts
    sti
syswait:
    jmp syswait


keyboard_handler:
    ; Read a byte from the device
    mov r0, 0x21
    mov r1, 0x1000
    cpucall

    ; Echo the byte back to the device
    mov r0, 0x20
    cpucall

    ; Return from the interrupt
    rti
