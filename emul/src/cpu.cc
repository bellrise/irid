/* CPU
   Copyright (c) 2023-2024 bellrise */

#include "emul.h"

#include <algorithm>
#include <cstring>
#include <stdio.h>
#include <sys/signal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static bool global_stop_cpu = false;

cpu::cpu(memory& memory)
    : m_mem(memory)
    , m_interrupts(false)
    , m_in_interrupt(false)
    , m_cycle_ns(0)
    , m_target_ips(0)
    , m_total_instructions(0)
    , m_devices()
{
    initialize();
}

cpu::~cpu() { }

void handle_ctrlc(int __attribute__((unused)) sig)
{
    /* If the CPU did not stop and we find ourselfs here again, force exit. */
    if (global_stop_cpu) {
        puts("\nForced exit.");
        exit(1);
    }

    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(STDIN_FILENO, 0, &term);

    global_stop_cpu = true;
}

void cpu::start()
{
    /* Disable canonical mode on stdin. */
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, 0, &term);

    signal(SIGINT, handle_ctrlc);

    clock_gettime(CLOCK_MONOTONIC, &m_start_time);

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

void cpu::set_target_ips(int target_ips)
{
    if (target_ips <= 0)
        return;

    m_cycle_ns = 1000000000 / target_ips;
    m_target_ips = target_ips;
}

void cpu::print_perf()
{
    struct timespec cur_time;
    double avg_ips;
    double avg_cycle;
    char prefix = ' ';

    clock_gettime(CLOCK_MONOTONIC, &cur_time);
    avg_ips = cur_time.tv_sec - m_start_time.tv_sec;
    if (avg_ips == 0)
        avg_ips = 1;
    avg_ips = m_total_instructions / avg_ips;

    if (avg_ips > 1000) {
        prefix = 'k';
        avg_ips /= 1000;
    }

    avg_cycle = (double) (cur_time.tv_sec - m_start_time.tv_sec)
              / m_total_instructions * 1000000;

    puts("\nCPU performance results:\n");
    printf("  total instructions    %zu\n", m_total_instructions);
    printf("  average IPS           %.2lf %.1sHz\n", avg_ips,
           prefix == ' ' ? "" : &prefix);
    printf("  average cycle time    %.2lf us\n", avg_cycle);
    printf("  target IPS            %d Hz\n", m_target_ips);
    fputc('\n', stdout);
}

void cpu::add_device(const device& dev)
{
    m_devices.push_back(dev);
}

void cpu::remove_devices()
{
    for (device& dev : m_devices) {
        if (dev.close)
            dev.close(dev);
    }

    m_devices.clear();
}

void cpu::mainloop()
{
    struct timespec instr_time_start;
    struct timespec instr_time_end;
    struct timespec sleep_time;
    int nsec;
    u8 instr;

    while (1) {
        /* Quick check if the user requested a shutdown. */

        if (global_stop_cpu)
            throw cpucall_request(cpucall_request::RQ_POWEROFF);

        /* Start the instruction cycle. */

        clock_gettime(CLOCK_MONOTONIC, &instr_time_start);

        /* Before we load & run another instruction, poll all devices for any
           incoming data. We also have to wait with polling the devices after
           we exit any currently-being-processed interrupts. */

        if (m_interrupts && !m_in_interrupt)
            poll_devices();

        instr = m_mem.read8(m_reg.ip);

        /* Run instruction. */
        switch (instr) {
        case I_CPUCALL:
            cpucall();
            break;
        case I_RTI:
            rti();
            goto dont_step;
        case I_STI:
            m_interrupts = true;
            break;
        case I_DSI:
            m_interrupts = false;
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
        case I_LOAD16:
            load16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_STORE16:
            store16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
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
        case I_CML:
            cml(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_CML8:
            cml8(m_mem.read8(m_reg.ip + 1), m_mem.read8(m_reg.ip + 2));
            break;
        case I_CML16:
            cml16(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            break;
        case I_JMP:
            jmp(m_mem.read16(m_reg.ip + 1));
            goto dont_step;
        case I_JNZ:
            jnz(m_mem.read8(m_reg.ip + 1), m_mem.read16(m_reg.ip + 2));
            goto dont_step;
        case I_JEQ:
            jeq(m_mem.read16(m_reg.ip + 1));
            goto dont_step;
        case I_CALL:
            call(m_mem.read16(m_reg.ip + 1));
            goto dont_step;
        case I_CALLR:
            callr(m_mem.read8(m_reg.ip + 1));
            goto dont_step;
        case I_RET:
            ret();
            goto dont_step;
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
dont_step:
        m_reg.ip += 0;

        /* Before we run the next instruction, check if we are not speeding
           and slow down to the target instructions-per-second appropriately. */

        clock_gettime(CLOCK_MONOTONIC, &instr_time_end);

        nsec = instr_time_end.tv_nsec - instr_time_start.tv_nsec;
        if (instr_time_end.tv_sec != instr_time_start.tv_sec) {
            nsec = instr_time_end.tv_nsec
                 + (1000000000 - instr_time_start.tv_nsec);
        }

        if (nsec < m_cycle_ns) {
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = m_cycle_ns - nsec;
            nanosleep(&sleep_time, NULL);
        }

        m_total_instructions++;
    }
}

void cpu::poll_devices()
{
    for (size_t i = 0; i < m_devices.size(); i++) {
        if (!m_devices[i].handlerptr || !m_devices[i].poll)
            continue;

        if (m_devices[i].poll(m_devices[i]))
            issue_interrupt(m_devices[i].handlerptr);
    }
}

void cpu::issue_interrupt(u16 addr)
{
    /* Before executing the interrupt function, the CPU caches all registers to
       be restored when rti is called. */

    m_in_interrupt = true;
    m_reg_cache = m_reg;
    m_reg.ip = addr;
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

u16 cpu::r_load(u8 id)
{
    if (is_half_register(id))
        return static_cast<u16>(*regptr<u8>(id));
    else
        return *regptr<u16>(id);
}

void cpu::r_store(u8 id, u16 value)
{
    if (is_half_register(id))
        *regptr<u8>(id) = static_cast<u8>(value);
    else
        *regptr<u16>(id) = value;
}

bool cpu::is_half_register(u8 id)
{
    return id >= R_H0 && id <= R_L3;
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
    case CPUCALL_DEVICELIST:
        cpucall_devicelist();
        break;
    case CPUCALL_DEVICEINFO:
        cpucall_deviceinfo();
        break;
    case CPUCALL_DEVICEINTR:
        cpucall_deviceintr();
        break;
    case CPUCALL_DEVICEWRITE:
        cpucall_devicewrite();
        break;
    case CPUCALL_DEVICEREAD:
        cpucall_deviceread();
        break;
    case CPUCALL_DEVICEPOLL:
        cpucall_devicepoll();
        break;
    default:
        throw cpu_fault(CPUFAULT_CPUCALL);
    }
}

void cpu::rti()
{
    m_in_interrupt = false;
    m_reg = m_reg_cache;
}

void cpu::cpucall_devicelist()
{
    u16 pointer = m_reg.r1;
    u16 maxlen = m_reg.r2;

    u16 to_write = std::min(m_devices.size(), (size_t) maxlen);

    u16 *ids = new u16[to_write];
    for (size_t i = 0; i < to_write; i++)
        ids[i] = m_devices[i].id;

    /* Fill the array with IDs. */
    m_mem.write_range(pointer, ids, to_write * sizeof(u16));

    /* Tell the user how many we've filled. */
    m_reg.r2 = to_write;

    delete[] ids;
}

void cpu::cpucall_deviceinfo()
{
    struct irid_deviceinfo info;

    for (size_t i = 0; i < m_devices.size(); i++) {
        if (m_devices[i].id != m_reg.r1)
            continue;

        info.d_id = m_devices[i].id;
        memset(info.d_name, 0, 14);
        memcpy(info.d_name, m_devices[i].name,
               std::min((size_t) 13, strlen(m_devices[i].name)));
        m_mem.write_range(m_reg.r2, &info, sizeof(info));
        break;
    }
}

void cpu::cpucall_deviceintr()
{
    for (size_t i = 0; i < m_devices.size(); i++) {
        if (m_devices[i].id != m_reg.r1)
            continue;

        m_devices[i].handlerptr = m_reg.r2;
        break;
    }
}

void cpu::cpucall_devicewrite()
{
    for (size_t i = 0; i < m_devices.size(); i++) {
        if (m_devices[i].id != m_reg.r1)
            continue;

        m_devices[i].write(m_devices[i], m_reg.h2);
        break;
    }
}

void cpu::cpucall_deviceread()
{
    for (size_t i = 0; i < m_devices.size(); i++) {
        if (m_devices[i].id != m_reg.r1)
            continue;

        m_reg.h2 = m_devices[i].read(m_devices[i]);
        break;
    }
}

void cpu::cpucall_devicepoll()
{
    for (size_t i = 0; i < m_devices.size(); i++) {
        if (m_devices[i].id != m_reg.r1)
            continue;

        m_reg.h2 = m_devices[i].poll(m_devices[i]);
        break;
    }
}

void cpu::push(u8 src)
{
    if (m_reg.sp == 0)
        throw cpu_fault(CPUFAULT_SEG);

    if (is_half_register(src)) {
        u8 val = *regptr<u8>(src);
        m_reg.sp -= 1;
        m_mem.write8(m_reg.sp, val);
    } else {
        u16 val = *regptr<u16>(src);
        m_reg.sp -= 2;
        m_mem.write16(m_reg.sp, val);
    }
}

void cpu::push8(u8 imm8)
{
    if (m_reg.sp == 0)
        throw cpu_fault(CPUFAULT_SEG);

    m_reg.sp -= 1;
    m_mem.write8(m_reg.sp, imm8);
}

void cpu::push16(u16 imm16)
{
    if (m_reg.sp == 0)
        throw cpu_fault(CPUFAULT_SEG);

    m_reg.sp -= 2;
    m_mem.write16(m_reg.sp, imm16);
}

void cpu::pop(u8 dest)
{
    if (is_half_register(dest)) {
        u8 val = m_mem.read8(m_reg.sp);
        m_reg.sp += 1;
        *regptr<u8>(dest) = val;
    } else {
        u16 val = m_mem.read16(m_reg.sp);
        m_reg.sp += 2;
        *regptr<u16>(dest) = val;
    }
}

void cpu::mov(u8 dest, u8 src)
{
    r_store(dest, r_load(src));
}

void cpu::mov8(u8 dest, u8 imm8)
{
    r_store(dest, imm8);
}

void cpu::mov16(u8 dest, u16 imm16)
{
    r_store(dest, imm16);
}

void cpu::load(u8 dest, u8 srcptr)
{
    if (is_half_register(srcptr))
        throw cpu_fault(CPUFAULT_REG);

    if (is_half_register(dest))
        r_store(dest, m_mem.read8(r_load(srcptr)));
    else
        r_store(dest, m_mem.read16(r_load(srcptr)));
}

void cpu::store(u8 src, u8 destptr)
{
    if (is_half_register(destptr))
        throw cpu_fault(CPUFAULT_REG);

    if (is_half_register(src))
        m_mem.write8(r_load(destptr), r_load(src));
    else
        m_mem.write16(r_load(destptr), r_load(src));
}

void cpu::load16(u8 dest, u16 imm16ptr)
{
    if (is_half_register(dest))
        r_store(dest, m_mem.read8(imm16ptr));
    else
        r_store(dest, m_mem.read16(imm16ptr));
}

void cpu::store16(u8 src, u16 imm16ptr)
{
    if (is_half_register(src))
        m_mem.write8(imm16ptr, r_load(src));
    else
        m_mem.write16(imm16ptr, r_load(src));
}

void cpu::null(u8 dest)
{
    r_store(dest, 0);
}

void cpu::cmp(u8 left, u8 right)
{
    m_reg.cf = r_load(left) == r_load(right);
}

void cpu::cmp8(u8 left, u8 imm8)
{
    m_reg.cf = r_load(left) == imm8;
}

void cpu::cmp16(u8 left, u16 imm16)
{
    m_reg.cf = r_load(left) == imm16;
}

void cpu::cmg(u8 left, u8 right)
{
    m_reg.cf = r_load(left) > r_load(right);
}

void cpu::cmg8(u8 left, u8 imm8)
{
    m_reg.cf = (u8) r_load(left) > imm8;
}

void cpu::cmg16(u8 left, u16 imm16)
{
    m_reg.cf = r_load(left) > imm16;
}

void cpu::cml(u8 left, u8 right)
{
    m_reg.cf = r_load(left) < r_load(right);
}

void cpu::cml8(u8 left, u8 imm8)
{
    m_reg.cf = (u8) r_load(left) < imm8;
}

void cpu::cml16(u8 left, u16 imm16)
{
    m_reg.cf = r_load(left) < imm16;
}

void cpu::jmp(u16 addr)
{
    m_reg.ip = addr;
}

void cpu::jnz(u8 cond, u16 addr)
{
    if (r_load(cond))
        m_reg.ip = addr;
    else
        m_reg.ip += 4;
}

void cpu::jeq(u16 addr)
{
    if (m_reg.cf)
        m_reg.ip = addr;
    else
        m_reg.ip += 4;
}

void cpu::call(u16 addr)
{
    push16(m_reg.ip + 4);
    m_reg.ip = addr;
}

void cpu::callr(u8 srcaddr)
{
    push16(m_reg.ip + 4);
    m_reg.ip = *regptr<u16>(srcaddr);
}

void cpu::ret()
{
    m_reg.ip = m_mem.read16(m_reg.sp);
    m_reg.sp += 2;
}

void cpu::add(u8 dest, u8 src)
{
    r_store(dest, r_load(dest) + r_load(src));
}

void cpu::add8(u8 dest, u8 imm8)
{
    r_store(dest, r_load(dest) + imm8);
}

void cpu::add16(u8 dest, u16 imm16)
{
    r_store(dest, r_load(dest) + imm16);
}

void cpu::sub(u8 dest, u8 src)
{
    r_store(dest, r_load(dest) - r_load(src));
}

void cpu::sub8(u8 dest, u8 imm8)
{
    r_store(dest, r_load(dest) - imm8);
}

void cpu::sub16(u8 dest, u16 imm16)
{
    r_store(dest, r_load(dest) - imm16);
}

void cpu::and_(u8 dest, u8 src)
{
    r_store(dest, r_load(dest) & r_load(src));
}

void cpu::and8(u8 dest, u8 imm8)
{
    r_store(dest, r_load(dest) & imm8);
}

void cpu::and16(u8 dest, u16 imm16)
{
    r_store(dest, r_load(dest) & imm16);
}

void cpu::or_(u8 dest, u8 src)
{
    r_store(dest, r_load(dest) | r_load(src));
}

void cpu::or8(u8 dest, u8 imm8)
{
    r_store(dest, r_load(dest) | imm8);
}

void cpu::or16(u8 dest, u16 imm16)
{
    r_store(dest, r_load(dest) | imm16);
}

void cpu::not_(u8 dest)
{
    r_store(dest, ~r_load(dest));
}

void cpu::shr(u8 dest, u8 src)
{
    if (is_half_register(dest))
        *regptr<u8>(dest) >>= *regptr<u8>(src);
    else
        *regptr<u16>(dest) >>= *regptr<u8>(src);
}

void cpu::shr8(u8 dest, u8 imm8)
{
    if (is_half_register(dest))
        *regptr<u8>(dest) >>= imm8;
    else
        *regptr<u16>(dest) >>= imm8;
}

void cpu::shl(u8 dest, u8 src)
{
    if (is_half_register(dest))
        *regptr<u8>(dest) <<= *regptr<u8>(src);
    else
        *regptr<u16>(dest) <<= *regptr<u8>(src);
}

void cpu::shl8(u8 dest, u8 imm8)
{
    if (is_half_register(dest))
        *regptr<u8>(dest) <<= imm8;
    else
        *regptr<u16>(dest) <<= imm8;
}

void cpu::mul(u8 dest, u8 src)
{
    if (is_half_register(dest)) {
        if (is_half_register(src))
            *regptr<u8>(dest) *= *regptr<u8>(src);
        else
            *regptr<u8>(dest) *= *regptr<u16>(src);
    } else {
        if (is_half_register(src))
            *regptr<u16>(dest) *= *regptr<u8>(src);
        else
            *regptr<u16>(dest) *= *regptr<u16>(src);
    }
}

void cpu::mul8(u8 dest, u8 imm8)
{
    if (is_half_register(dest))
        *regptr<u8>(dest) *= imm8;
    else
        *regptr<u16>(dest) *= imm8;
}

void cpu::mul16(u8 dest, u16 imm16)
{
    if (is_half_register(dest))
        *regptr<u8>(dest) *= imm16;
    else
        *regptr<u16>(dest) *= imm16;
}
