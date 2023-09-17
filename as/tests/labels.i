restart:
    cpucall
    jmp L2
L1:
    cpucall
    cpucall
    jmp L1
L2:
    cpucall
    jmp restart
