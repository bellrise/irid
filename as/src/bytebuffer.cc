/* Irid assembler
   Copyright (c) 2023 bellrise */

#include "as.h"

#include <cstring>
#include <stdexcept>

bytebuffer::bytebuffer()
    : m_alloc(nullptr)
    , m_size(0)
    , m_space(0)
{ }

bytebuffer::bytebuffer(const bytebuffer& copy)
{
    ensure_size(copy.m_size);
    std::memcpy(m_alloc, copy.m_alloc, copy.m_size);
    m_size = copy.m_size;
}

bytebuffer::bytebuffer(bytebuffer&& moved)
    : m_alloc(moved.m_alloc)
    , m_size(moved.m_size)
    , m_space(moved.m_space)
{
    moved.m_alloc = nullptr;
    moved.m_size = 0;
    moved.m_space = 0;
}

bytebuffer::~bytebuffer()
{
    clear();
}

size_t bytebuffer::len() const
{
    return m_size;
}

void bytebuffer::clear()
{
    delete[] m_alloc;
    m_alloc = nullptr;
    m_space = 0;
    m_size = 0;
}

void bytebuffer::append(std::byte byte)
{
    ensure_size(m_size + 1);
    m_alloc[m_size++] = byte;
}

void bytebuffer::append_range(range<std::byte>& bytes)
{
    ensure_size(m_size + bytes.len);
    std::memcpy(&m_alloc[m_size], bytes.ptr, bytes.len);
    m_size += bytes.len;
}

void bytebuffer::insert(std::byte byte, size_t index)
{
    ensure_size(index + 1);
    m_alloc[index] = byte;
    m_size = std::max(m_size, index + 1);
}

void bytebuffer::insert_range(range<std::byte>& bytes, size_t starting_index)
{
    ensure_size(starting_index + bytes.len);
    std::memcpy(&m_alloc[starting_index], bytes.ptr, bytes.len);
    m_size = std::max(m_size, starting_index + bytes.len);
}

std::byte bytebuffer::at(size_t index) const
{
    if (index >= m_size)
        throw std::out_of_range("index in at() out of range");
    return m_alloc[index];
}

range<std::byte> bytebuffer::get_range(size_t starting_index, size_t len) const
{
    size_t range_len;

    if (m_size == 0)
        return range<std::byte>(nullptr, 0);

    if (starting_index >= m_size)
        throw std::out_of_range("starting index in get_range() out of range");

    range_len = len;
    if (starting_index + range_len > m_size)
        range_len = m_size - starting_index;

    return range<std::byte>(&m_alloc[starting_index], range_len);
}

std::byte *bytebuffer::checked_new(size_t allocation_size)
{
    std::byte *ptr;

    try {
        ptr = new std::byte[allocation_size];
        std::memset(ptr, 0, allocation_size);
        return ptr;
    } catch (const std::bad_alloc&) {
        throw std::runtime_error("failed to allocate memory for bytebuffer");
    }
}

void bytebuffer::ensure_size(size_t required_size)
{
    size_t allocation_alignment;
    size_t allocation_size;
    std::byte *old_pointer;

    if (required_size <= m_space)
        return;

    /* Always allocate to the nearest alignment. */

    allocation_alignment = 64;
    if (required_size >= 1024)
        allocation_alignment = 1024;

    allocation_size =
        (required_size / allocation_alignment + 1) * allocation_alignment;

    if (!m_space) {
        m_alloc = checked_new(allocation_size);
        m_space = allocation_size;
        return;
    }

    /* Re-allocate buffer. */

    old_pointer = m_alloc;
    m_alloc = checked_new(allocation_size);
    m_space = allocation_size;
    std::memcpy(m_alloc, old_pointer, m_size);
    delete[] old_pointer;
}
