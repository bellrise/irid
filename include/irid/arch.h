/* Irid architecture instruction set.
   Copyright (C) 2021-2024 bellrise */

#ifndef IRID_ARCH_H
#define IRID_ARCH_H

#include <stdint.h>

#define IRID_PAGE_SIZE      0x0400
#define IRID_MAX_ADDR       0xffff
#define IRID_MAX_PAGES      (IRID_MAX_ADDR + 1) / IRID_PAGE_SIZE
#define IRID_PAGE_SIZE_BITS 10
#define IRID_PAGE_MASK      0xfc00
#define IRID_PTR_WIDTH      16

/*
 * Irid regular int & half int. All full 16-bit registers use the rint type
 * instead of the iint type, because the signed bit needs to be ignored.
 */

#ifndef IRID_DEFINED_UX
# define IRID_DEFINED_UX 1
typedef unsigned short u16;
typedef unsigned char u8;
#endif

/*
 * Register IDs. In the instruction format, registers are represented using
 * a single byte.
 */

#define R_R0 0x00
#define R_R1 0x01
#define R_R2 0x02
#define R_R3 0x03
#define R_R4 0x04
#define R_R5 0x05
#define R_R6 0x06
#define R_R7 0x07
#define R_H0 0x10
#define R_L0 0x20
#define R_H1 0x11
#define R_L1 0x21
#define R_H2 0x12
#define R_L2 0x22
#define R_H3 0x13
#define R_L3 0x23
#define R_IP 0x70
#define R_SP 0x71
#define R_BP 0x72

/*
 * Instruction set. All instructions fit in the 0-255 range, all fitting in
 * a single byte. For more information on each instruction, see doc/arch.
 */

#define I_NOP     0x00
#define I_CPUCALL 0x01
#define I_RTI     0x02
#define I_STI     0x03
#define I_DSI     0x04
/* reserved */
#define I_PUSH    0x10
#define I_PUSH8   0x11
#define I_PUSH16  0x12
#define I_POP     0x13
#define I_MOV     0x14
#define I_MOV8    0x15
#define I_MOV16   0x16
#define I_LOAD    0x17
#define I_STORE   0x18
#define I_NULL    0x19
#define I_CMP     0x1a
#define I_CMP8    0x1b
#define I_CMP16   0x1c
#define I_CMG     0x1d
#define I_CMG8    0x1e
#define I_CMG16   0x1f
#define I_CML     0x20
#define I_CML8    0x21
#define I_CML16   0x22
#define I_LOAD16  0x23
#define I_STORE16 0x24
#define I_CFS     0x25
/* reserved */
#define I_JMP   0x30
#define I_JNZ   0x31
#define I_JEQ   0x32
#define I_CALL  0x33
#define I_CALLR 0x34
#define I_RET   0x35
#define I_ADD   0x36
#define I_ADD8  0x37
#define I_ADD16 0x38
#define I_SUB   0x39
#define I_SUB8  0x3a
#define I_SUB16 0x3b
#define I_AND   0x3c
#define I_AND8  0x3d
#define I_AND16 0x3e
#define I_OR    0x3f
#define I_OR8   0x40
#define I_OR16  0x41
#define I_NOT   0x42
#define I_SHR   0x43
#define I_SHR8  0x44
#define I_SHL   0x45
#define I_SHL8  0x46
#define I_MUL   0x47
#define I_MUL8  0x48
#define I_MUL16 0x49

/*
 * CPU call functions. In order to execute a function built into the CPU itself,
 * call the I_CPUCALL instruction with the correct function in r0.
 */

#define CPUCALL_POWEROFF    0x10
#define CPUCALL_RESTART     0x11
#define CPUCALL_FAULT       0x12
#define CPUCALL_DEVICELIST  0x13
#define CPUCALL_DEVICEINFO  0x14
#define CPUCALL_DEVICEINTR  0x15
#define CPUCALL_DEVICEWRITE 0x20
#define CPUCALL_DEVICEREAD  0x21
#define CPUCALL_DEVICEPOLL  0x22

struct irid_deviceinfo
{
    u16 d_id;
    u8 d_name[14];
};

/*
 * CPU fault IDs. Each possible CPU fault has a corresponding 8-bit number,
 * which can be used to identify the problem.
 */

#define CPUFAULT_SEG     0x01 /* Segmentation fault */
#define CPUFAULT_IO      0x02 /* IO fault */
#define CPUFAULT_STACK   0x03 /* Stack corruption */
#define CPUFAULT_REG     0x04 /* Invalid register */
#define CPUFAULT_INS     0x05 /* Illegal instruction */
#define CPUFAULT_USER    0x06 /* Forced fault */
#define CPUFAULT_CPUCALL 0x07 /* Invalid CPU call */

#define _irid_joined_register(ID)                                              \
    union                                                                      \
    {                                                                          \
        u16 r##ID;                                                             \
        struct                                                                 \
        {                                                                      \
            u8 h##ID;                                                          \
            u8 l##ID;                                                          \
        };                                                                     \
    }

/* Irid CPU; registers & flags. */

struct irid_reg
{
    /* u16 r0, r1, r2, r3 */
    /* u8 h0, h1, h2, h3 */
    /* u8 l0, l1, l2, l3 */
    _irid_joined_register(0);
    _irid_joined_register(1);
    _irid_joined_register(2);
    _irid_joined_register(3);

    u16 r4, r5, r6, r7;
    u16 ip, sp, bp; /* Instruction, stack & base ptr */

    struct
    {
        u16 cf : 1; /* Compare flag */
        u16 zf : 1; /* Zero flag */
        u16 of : 1; /* Overflow flag */
        u16 sf : 1; /* Shutdown flag */
    };
};

#undef _irid_joined_register

#endif /* IRID_ARCH_H */
