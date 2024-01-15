/* Memory
   Copyright (c) 2023-2024 bellrise */

#include "emul.h"

#include <cstring>
#include <stdexcept>
#include <sys/mman.h>

memory::memory(size_t total_size, size_t __attribute__((unused)) page_size)
    : m_totalsize(total_size)
{
    m_mem = (uint8_t *) mmap(NULL, m_totalsize, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANON, -1, 0);
    if (m_mem == MAP_FAILED)
        throw std::runtime_error("failed host mmap()");
    std::memset(m_mem, 0, m_totalsize);
}

memory::~memory()
{
    munmap(m_mem, m_totalsize);
}

u8 memory::read8(u16 addr)
{
    checkaddr(addr);
    return m_mem[addr];
}

void memory::write8(u16 addr, u8 value)
{
    checkaddr(addr);
    m_mem[addr] = value;
}

u16 memory::read16(u16 addr)
{
    checkaddr(addr);
    return m_mem[addr] | (m_mem[addr + 1] << 8);
}

void memory::write16(u16 addr, u16 value)
{
    checkaddr(addr);
    m_mem[addr] = value & 0xff;
    m_mem[addr + 1] = (value & 0xff00) >> 8;
}

void memory::read_range(u16 src, void *dest, u16 n)
{
    std::memcpy(dest, &m_mem[src], n);
}

void memory::write_range(u16 dest, void *src, u16 n)
{
    std::memcpy(&m_mem[dest], src, n);
}

void memory::dump(u16 addr, u16 n)
{
    dbytes(&m_mem[addr], n);
}

inline void memory::checkaddr(u16 addr)
{
    if (addr >= m_totalsize)
        throw cpu_fault(CPUFAULT_SEG);
}
