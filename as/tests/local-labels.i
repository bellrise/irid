restart:
    cpucall
    jmp @local
@local:
    cpucall

other_func:
@local:
    cpucall
    jmp @local
