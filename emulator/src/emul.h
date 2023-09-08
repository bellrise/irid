/* irid-emul
   Copyright (c) 2023 bellrise */

#pragma once

#include "arch.h"

#include <functional>
#include <stddef.h>
#include <stdexcept>
#include <vector>

#define IRID_EMUL_VERSION "0.5"

#ifdef DEBUG
/* info() is only called if DEBUG is defined. */
# define info(...) _info_impl("irid-emul", __func__, __VA_ARGS__)
#else
# define info(...)
#endif

#define die(...)  _die_impl("irid-emul", __VA_ARGS__)
#define warn(...) _warn_impl("irid-emul", __VA_ARGS__)

/* Thrown on a CPU fault. */
struct cpu_fault
{
    cpu_fault(int fault_type)
        : fault(fault_type)
    { }

    /* One of CPUFAULT_* */
    int fault;
};

struct cpucall_request
{
    enum rq
    {
        RQ_POWEROFF,
        RQ_RESTART
    };

    cpucall_request(rq rq_type)
        : request(rq_type)
    { }

    rq request;
};

/* Provides a memory layout & access mechanisms. */
struct memory
{
    memory(size_t total_size, size_t page_size);
    ~memory();

    /* Read/write bytes from/to memory. May throw page_fault. */
    u8 read8(u16 addr);
    void write8(u16 addr, u8 value);
    u16 read16(u16 addr);
    void write16(u16 addr, u16 value);

    void read_range(u16 src, void *dest, u16 n);
    void write_range(u16 dest, void *src, u16 n);

    void dump(u16 addr, u16 n);

  private:
    size_t m_totalsize;
    uint8_t *m_mem;

    void checkaddr(u16 addr);
};

struct device;

struct cpu
{
    cpu(memory& memory);
    ~cpu();

    void start();
    void set_target_ips(int target_ips);
    void print_perf();

    void add_device(const device& dev);

  private:
    memory& m_mem;
    irid_reg m_reg;
    irid_reg m_reg_cache;
    bool m_interrupts;
    bool m_in_interrupt;
    int m_cycle_ns;
    int m_target_ips;
    size_t m_total_instructions;
    std::vector<device> m_devices;
    struct timespec m_start_time;

    void initialize();
    void mainloop();
    void poll_devices();
    void issue_interrupt(u16 addr);
    void dump_registers();

    /* Register manipulation */
    u16 r_load(u8 id);
    void r_store(u8 id, u16 value);

    /* CPU instructions. */
    void cpucall();
    void rti();
    void push(u8 src);
    void push8(u8 imm8);
    void push16(u16 imm16);
    void pop(u8 dest);
    void mov(u8 dest, u8 src);
    void mov8(u8 dest, u8 imm8);
    void mov16(u8 dest, u16 imm16);
    void load(u8 dest, u8 srcptr);
    void store(u8 src, u8 destptr);
    void load16(u8 dest, u16 imm16ptr);
    void store16(u8 src, u16 imm16ptr);
    void null(u8 dest);
    void cmp(u8 left, u8 right);
    void cmp8(u8 left, u8 imm8);
    void cmp16(u8 left, u16 imm16);
    void cmg(u8 left, u8 right);
    void cmg8(u8 left, u8 imm8);
    void cmg16(u8 left, u16 imm16);
    void cml(u8 left, u8 right);
    void cml8(u8 left, u8 imm8);
    void cml16(u8 left, u16 imm16);
    void jmp(u16 addr);
    void jnz(u8 cond, u16 addr);
    void jeq(u16 addr);
    void call(u16 addr);
    void callr(u8 srcaddr);
    void ret();
    void add(u8 dest, u8 src);
    void add8(u8 dest, u8 imm8);
    void add16(u8 dest, u16 imm16);
    void sub(u8 dest, u8 src);
    void sub8(u8 dest, u8 imm8);
    void sub16(u8 dest, u16 imm16);
    void and_(u8 dest, u8 src);
    void and8(u8 dest, u8 imm8);
    void and16(u8 dest, u16 imm16);
    void or_(u8 dest, u8 src);
    void or8(u8 dest, u8 imm8);
    void or16(u8 dest, u16 imm16);
    void not_(u8 dest);
    void shr(u8 dest, u8 src);
    void shr8(u8 dest, u8 imm8);
    void shl(u8 dest, u8 src);
    void shl8(u8 dest, u8 imm8);
    void mul(u8 dest, u8 src);
    void mul8(u8 dest, u8 imm8);
    void mul16(u8 dest, u16 imm16);

    void cpucall_devicelist();
    void cpucall_deviceinfo();
    void cpucall_deviceintr();
    void cpucall_devicewrite();
    void cpucall_deviceread();

    template <typename T>
    T *regptr(u8 id)
    {
        if (id == R_IP)
            return reinterpret_cast<T *>(&m_reg.ip);
        if (id == R_SP)
            return reinterpret_cast<T *>(&m_reg.sp);
        if (id == R_BP)
            return reinterpret_cast<T *>(&m_reg.bp);

        if (id < 0x10)
            return reinterpret_cast<T *>(&((u16 *) &m_reg)[id]);
        if (id < 0x20)
            return reinterpret_cast<T *>(regptr<u8>(id - 0x10));
        return reinterpret_cast<T *>(regptr<u8>(id - 0x20) + 1);
    }

    bool is_half_register(u8 id);
};

struct device
{
    u16 id;
    u16 handlerptr;
    const char *name;

    device(u16 id, const char *name);

    std::function<void(u8)> write;
    std::function<u8(void)> read;
    std::function<bool(void)> poll;
};

/* Dump `amount` bytes starting from `addr` to stdout. */
void dbytes(void *addr, size_t amount);

int _die_impl(const char *prog, const char *fmt, ...);
int _warn_impl(const char *prog, const char *fmt, ...);
int _info_impl(const char *prog, const char *func, const char *fmt, ...);
