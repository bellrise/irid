/* Irid architecture instruction set.
   Copyright (C) 2021-2022 bellrise */

#ifndef IRID_ARCH_H
#define IRID_ARCH_H

#include <stdint.h>

#define IRID_PAGE_SIZE          0x0400
#define IRID_MAX_ADDR           0xffff
#define IRID_MAX_PAGES          (IRID_MAX_ADDR + 1) / IRID_PAGE_SIZE
#define IRID_PAGE_SIZE_BITS     10

/*
 * Register IDs. In the instruction format, registers are represented using
 * a single byte.
 */

#define R_R0                    0x00
#define R_R1                    0x01
#define R_R2                    0x02
#define R_R3                    0x03
#define R_R4                    0x04
#define R_R5                    0x05
#define R_R6                    0x06
#define R_R7                    0x07
#define R_H0                    0x10
#define R_L0                    0x11
#define R_H1                    0x12
#define R_L1                    0x13
#define R_H2                    0x14
#define R_L2                    0x15
#define R_H3                    0x16
#define R_L3                    0x17
#define R_IP                    0x70
#define R_SP                    0x71
#define R_BP                    0x72

/*
 * Instruction set. All instructions fit in the 0-255 range, all fitting in
 * a single byte. For more information on each instruction, see doc/arch.
 */

#define I_NOP                   0x00
#define I_CPUCALL               0x01
/* reserved */
#define I_PUSH                  0x10
#define I_PUSH8                 0x11
#define I_PUSH16                0x12
#define I_POP                   0x13
#define I_MOV                   0x14
#define I_MOV8                  0x15
#define I_MOV16                 0x16
#define I_LOAD                  0x17
#define I_STORE                 0x18
#define I_NULL                  0x19
#define I_CMP                   0x1a
#define I_CMP8                  0x1b
#define I_CMP16                 0x1c
#define I_CMG                   0x1d
#define I_CMG8                  0x1e
#define I_CMG16                 0x1f
/* reserved */
#define I_JMP                   0x30
#define I_JNZ                   0x31
#define I_JCS                   0x32
#define I_CALL                  0x33
#define I_RET                   0x34
#define I_ADD                   0x35
#define I_ADD8                  0x36
#define I_ADD16                 0x37
#define I_SUB                   0x38
#define I_SUB8                  0x39
#define I_SUB16                 0x3a
#define I_AND                   0x3b
#define I_AND8                  0x3c
#define I_AND16                 0x3d
#define I_OR                    0x3e
#define I_OR8                   0x3f
#define I_OR16                  0x40
#define I_NOT                   0x41
#define I_SHR                   0x42
#define I_SHR8                  0x43
#define I_SHL                   0x44
#define I_SHL8                  0x45

/*
 * CPU call functions. In order to execute a function built into the CPU itself,
 * call the I_CPUCALL instruction with the correct function in r0.
 */

#define CPUCALL_SHUTDOWN        0x10
#define CPUCALL_RESTART         0x11
#define CPUCALL_LISTDEVS        0x12
#define CPUCALL_VMOD            0x13
#define CPUCALL_FAULT           0x14

/*
 * Visual mode of the CPU. You can set this using the CPUCALL_VMOD function.
 * If the text mode is enabled, the screen is rendered as a 80*25 character map,
 * which you can modify writing to video memory. This is on by default. The
 * pixel mode changes the screen resolution to the values given in r2 & r3.
 * You can learn more in doc/arch.
 */

#define CPUCALL_VMOD_TXT        0x01
#define CPUCALL_VMOD_PIX        0x02

/*
 * CPU fault IDs. Each possible CPU fault has a corresponding 8-bit number,
 * which can be used to identify the problem.
 */

#define CPUFAULT_SEG            0x01        /* Segmentation fault */
#define CPUFAULT_IO             0x02        /* IO fault */
#define CPUFAULT_STACK          0x03        /* Stack corruption */
#define CPUFAULT_REG            0x04        /* Invalid register */
#define CPUFAULT_INS            0x05        /* Illegal instruction */
#define CPUFAULT_USER           0x06        /* Forced fault */
#define CPUFAULT_CPUCALL        0x07        /* Invalid CPU call */

/*
 * Irid regular int & half int. All full 16-bit registers use the rint type
 * instead of the iint type, because the signed bit needs to be ignored.
 */

typedef short           ir_sword;
typedef unsigned short  ir_word;
typedef unsigned char   ir_half;


#define _irid_join_register(ID) union { ir_word r##ID; struct {             \
	ir_half    h ## ID;                                                     \
	ir_half    l ## ID;                                                     \
}; }

/* Irid register set. Note that the placement is intentional. */

struct irid_reg
{
	/* ir_word r0, r1, r2, r3 */
	/* ir_half h0, h1, h2, h3 */
	/* ir_half l0, l1, l2, l3 */
	_irid_join_register(0);
	_irid_join_register(1);
	_irid_join_register(2);
	_irid_join_register(3);

	ir_word r4, r5, r6, r7;
	ir_word ip, sp, bp;         /* Instruction, stack & base ptr */

	struct
	{
		ir_word    cf : 1;      /* Compare flag */
		ir_word    zf : 1;      /* Zero flag */
		ir_word    of : 1;      /* Overflow flag */
		ir_word    sf : 1;      /* Shutdown flag */
	};
};

#undef _irid_join_register

#endif /* IRID_ARCH_H */
