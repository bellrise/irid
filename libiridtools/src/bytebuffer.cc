/* libiridtools
   Copyright (c) 2023-2024 bellrise */

#include <cstring>
#include <libiridtools/bytebuffer.h>
#include <stdexcept>

bytebuffer::bytebuffer()
    : m_alloc(nullptr)
    , m_size(0)
    , m_space(0)
    , m_frozen(false)
{ }

bytebuffer::bytebuffer(const bytebuffer& copy)
    : m_alloc(nullptr)
    , m_size(0)
    , m_space(0)
    , m_frozen(false)
{
    *this = copy;
}

bytebuffer::bytebuffer(bytebuffer&& moved)
    : m_alloc(moved.m_alloc)
    , m_size(moved.m_size)
    , m_space(moved.m_space)
    , m_frozen(moved.m_frozen)
{
    moved.m_alloc = nullptr;
    moved.m_size = 0;
    moved.m_space = 0;
    moved.m_frozen = false;
}

bytebuffer::~bytebuffer()
{
    unfreeze();
    clear();
}

size_t bytebuffer::len() const
{
    return m_size;
}

void bytebuffer::freeze()
{
    m_frozen = true;
}

void bytebuffer::unfreeze()
{
    m_frozen = false;
}

void bytebuffer::clear()
{
    check_freeze();
    delete[] m_alloc;
    m_alloc = nullptr;
    m_space = 0;
    m_size = 0;
}

void bytebuffer::append(byte byte)
{
    check_freeze();
    ensure_size(m_size + 1);
    m_alloc[m_size++] = byte;
}

void bytebuffer::append_range(range<byte> bytes)
{
    check_freeze();
    ensure_size(m_size + bytes.len);
    std::memcpy(&m_alloc[m_size], bytes.ptr, bytes.len);
    m_size += bytes.len;
}

void bytebuffer::append_buffer(const bytebuffer& buf)
{
    check_freeze();
    append_range(buf.get_range(0, buf.len()));
}

void bytebuffer::insert(byte byte, size_t index)
{
    check_freeze();
    ensure_size(index + 1);
    m_alloc[index] = byte;
    m_size = std::max(m_size, index + 1);
}

void bytebuffer::insert_range(range<byte> bytes, size_t starting_index)
{
    check_freeze();
    ensure_size(starting_index + bytes.len);
    std::memcpy(&m_alloc[starting_index], bytes.ptr, bytes.len);
    m_size = std::max(m_size, starting_index + bytes.len);
}

void bytebuffer::insert_fill(byte with_byte, size_t starting_index, size_t len)
{
    check_freeze();
    ensure_size(starting_index + len);
    std::memset(&m_alloc[starting_index], with_byte, len);
    m_size = std::max(m_size, starting_index + len);
}

byte bytebuffer::at(size_t index) const
{
    if (index >= m_size)
        throw std::out_of_range("index in at() out of range");
    return m_alloc[index];
}

range<byte> bytebuffer::get_range(size_t starting_index, size_t len) const
{
    size_t range_len;

    if (m_size == 0)
        return range<byte>(nullptr, 0);

    if (starting_index >= m_size)
        throw std::out_of_range("starting index in get_range() out of range");

    range_len = len;
    if (starting_index + range_len > m_size)
        range_len = m_size - starting_index;

    return range<byte>(&m_alloc[starting_index], range_len);
}

void bytebuffer::copy_out(byte *buffer, size_t starting_index, size_t len) const
{
    size_t to_copy = len;

    if (starting_index >= m_size)
        throw std::out_of_range("starting index in copy_out() out of range");

    if (starting_index + to_copy > m_size)
        to_copy = m_size - starting_index;

    memcpy((void *) buffer, m_alloc, to_copy);
}

bytebuffer& bytebuffer::operator=(const bytebuffer& copy)
{
    check_freeze();
    ensure_size(copy.m_size);
    std::memcpy(m_alloc, copy.m_alloc, copy.m_size);
    m_size = copy.m_size;
    return *this;
}

void bytebuffer::check_freeze()
{
    if (m_frozen)
        throw std::runtime_error("tried to modify frozen bytebuffer");
}

byte *bytebuffer::checked_new(size_t allocation_size)
{
    byte *ptr;

    try {
        ptr = new byte[allocation_size];
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
    byte *old_pointer;

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
