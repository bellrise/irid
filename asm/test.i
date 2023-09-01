; Test script

.org 0x0000

restart:
    ; Set up stack at 0x1000
    mov sp, 0x1000

    ; Force the CPU to fault
    mov r0, 0x12
    cpucall

.string "hello"
