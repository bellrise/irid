; cpucall.i
; Communication with the CPU.
; Copyright (c) 2024 bellrise

.export __cpucall
.export __cpucallh

; Call the internal CPU functions with the cpucall instruction.
; __cpucall(int function, ...)
__cpucall:
    cpucall
    mov r0, r2
    ret

; Call the internal CPU functions with the cpucall instruction, return
; only the upper half of the r2 register.
; __cpucallh(int function, ...)
__cpucallh:
    cpucall
    mov r0, 0
    mov h0, h2
    ret
