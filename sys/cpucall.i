; cpucall.i
; Communication with the CPU.
; Copyright (c) 2024 bellrise

.export __cpucall

; Call the internal CPU functions with the cpucall instruction.
; __cpucall(int function, ...)
__cpucall:
    cpucall
    mov r0, r2
    ret
