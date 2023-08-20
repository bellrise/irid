/* CPU
   Copyright (c) 2023 bellrise */

#include "emul.h"

#include <cstring>

cpu::cpu(memory& memory)
    : m_mem(memory)
{
    initialize();
}

cpu::~cpu() { }

void cpu::start()
{
    while (1) {
        try {
            mainloop();
        } catch (const cpu_fault& fault) {
            dump_registers();
            die("CPU fault: %x", fault.fault);
        } catch (const cpucall_request& rq) {
            if (rq.request == rq.RQ_RESTART) {
                initialize();
                continue;
            } else if (rq.request == rq.RQ_POWEROFF) {
                break;
            }
        }
    }
}

void cpu::mainloop()
{
    u8 instr;

    while (1) {
        instr = m_mem.read8(m_reg.ip);

        info("instr=%x", instr);

        /* Run instruction. */
        switch (instr) {
        case I_CPUCALL:
            cpucall();
            break;
        case I_PUSH:
            push(m_mem.read8(m_reg.ip + 1));
            break;
        case I_PUSH8:
            push8(m_mem.read8(m_reg.ip + 1));
            break;
        case I_PUSH16:
            push16(m_mem.read16(m_reg.ip + 1));
            break;
        case I_POP:
            pop(m_mem.read8(m_reg.ip + 1));
            break;
        case I_MOV:
            mov(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_MOV8:
            mov8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_MOV16:
            mov16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_LOAD:
            load(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_STORE:
            store(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_NULL:
            null(m_mem.read8(m_reg.ip + 1));
            break;
        case I_CMP:
            cmp(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_CMP8:
            cmp8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_CMP16:
            cmp16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_CMG:
            cmg(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_CMG8:
            cmg8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_CMG16:
            cmg16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_JMP:
            jmp(m_mem.read16(m_reg.ip + 1));
            break;
        case I_JNZ:
            jnz(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_JEQ:
            jeq(m_mem.read16(m_reg.ip + 1));
            break;
        case I_CALL:
            call(m_mem.read16(m_reg.ip + 1));
            break;
        case I_CALLR:
            callr(m_mem.read8(m_reg.ip + 1));
            break;
        case I_RET:
            ret();
            break;
        case I_ADD:
            add(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_ADD8:
            add8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_ADD16:
            add16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_SUB:
            sub(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_SUB8:
            sub8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_SUB16:
            sub16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_AND:
            and_(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_AND8:
            and8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_AND16:
            and16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_OR:
            or_(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_OR8:
            or8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_OR16:
            or16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_NOT:
            not_(m_mem.read8(m_reg.ip + 1));
            break;
        case I_SHR:
            shr(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_SHR8:
            shr8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_SHL:
            shl(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_SHL8:
            shl8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_MUL:
            mul(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_MUL8:
            mul8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_MUL16:
            mul16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        default:
            break;
        }

        m_reg.ip += 4;
    }
}

void cpu::dump_registers()
{
    printf("\n\033[1;31mCPU fault\033[0m\n\n");
    printf("Registers:\n"
           "  r0=0x%04x r1=0x%04x r2=0x%04x r3=0x%04x\n"
           "  r4=0x%04x r5=0x%04x r6=0x%04x r7=0x%04x\n"
           "  ip=0x%04x sp=0x%04x bp=0x%04x\n"
           "  cf=%d      zf=%d      of=%d      sf=%d\n\n",
           m_reg.r0, m_reg.r1, m_reg.r2, m_reg.r3, m_reg.r4, m_reg.r5, m_reg.r6,
           m_reg.r7, m_reg.ip, m_reg.sp, m_reg.bp, m_reg.cf, m_reg.zf, m_reg.of,
           m_reg.sf);
}

void cpu::initialize()
{
    std::memset(&m_reg, 0, sizeof(m_reg));
}

void cpu::cpucall()
{
    switch (m_reg.r0) {
    case CPUCALL_POWEROFF:
        throw cpucall_request(cpucall_request::RQ_POWEROFF);
    case CPUCALL_RESTART:
        throw cpucall_request(cpucall_request::RQ_RESTART);
    case CPUCALL_FAULT:
        throw cpu_fault(CPUFAULT_USER);
    default:
        throw cpu_fault(CPUFAULT_CPUCALL);
    }
}

void cpu::push(u8 src) { }
void cpu::push8(u8 imm8) { }
void cpu::push16(u16 imm16) { }
void cpu::pop(u8 dest) { }

void cpu::mov(u8 dest, u8 src)
{
    *regptr<u16>(dest) = *regptr<u16>(src);
}

void cpu::mov8(u8 dest, u8 imm8)
{
    *regptr<u8>(dest) = imm8;
}

void cpu::mov16(u8 dest, u16 imm16)
{
    *regptr<u16>(dest) = imm16;
}

void cpu::load(u8 dest, u8 srcptr) { }
void cpu::store(u8 src, u8 destptr) { }
void cpu::null(u8 dest) { }
void cpu::cmp(u8 left, u8 right) { }
void cpu::cmp8(u8 left, u8 imm8) { }
void cpu::cmp16(u8 left, u16 imm16) { }
void cpu::cmg(u8 left, u8 right) { }
void cpu::cmg8(u8 left, u8 imm8) { }
void cpu::cmg16(u8 left, u16 imm16) { }
void cpu::jmp(u16 addr) { }
void cpu::jnz(u8 cond, u16 addr) { }
void cpu::jeq(u16 addr) { }
void cpu::call(u16 addr) { }
void cpu::callr(u8 srcaddr) { }
void cpu::ret() { }
void cpu::add(u8 dest, u8 src) { }
void cpu::add8(u8 dest, u8 imm8) { }
void cpu::add16(u8 dest, u16 imm16) { }
void cpu::sub(u8 dest, u8 src) { }
void cpu::sub8(u8 dest, u8 imm8) { }
void cpu::sub16(u8 dest, u16 imm16) { }
void cpu::and_(u8 dest, u8 src) { }
void cpu::and8(u8 dest, u8 imm8) { }
void cpu::and16(u8 dest, u16 imm16) { }
void cpu::or_(u8 dest, u8 src) { }
void cpu::or8(u8 dest, u8 imm8) { }
void cpu::or16(u8 dest, u16 imm16) { }
void cpu::not_(u8 dest) { }
void cpu::shr(u8 dest, u8 src) { }
void cpu::shr8(u8 dest, u8 imm8) { }
void cpu::shl(u8 dest, u8 src) { }
void cpu::shl8(u8 dest, u8 imm8) { }
void cpu::mul(u8 dest, u8 src) { }
void cpu::mul8(u8 dest, u8 imm8) { }
void cpu::mul16(u8 dest, u16 imm16) { }
