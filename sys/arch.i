; Irid architecture macro values, possibly useful in programs.
; Copyright (c) 2023-2024 bellrise

; Functions built-into the CPU itself.

.value CPUCALL_POWEROFF    0x10
.value CPUCALL_RESTART     0x11
.value CPUCALL_FAULT       0x12
.value CPUCALL_DEVICELIST  0x13
.value CPUCALL_DEVICEINFO  0x14
.value CPUCALL_DEVICEINTR  0x15
.value CPUCALL_DEVICEWRITE 0x20
.value CPUCALL_DEVICEREAD  0x21
.value CPUCALL_DEVICEPOLL  0x22

; CPU fault numbers

.value CPUFAULT_SEG     0x01 ; Segmentation fault
.value CPUFAULT_IO      0x02 ; IO fault
.value CPUFAULT_STACK   0x03 ; Stack corruption
.value CPUFAULT_REG     0x04 ; Invalid register
.value CPUFAULT_INS     0x05 ; Illegal instruction
.value CPUFAULT_USER    0x06 ; Forced fault
.value CPUFAULT_CPUCALL 0x07 ; Invalid CPU call

; Console

.value CCTL_MODE 0x11
.value CCTL_SIZE 0x01
