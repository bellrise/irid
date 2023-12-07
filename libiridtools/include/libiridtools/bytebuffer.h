/* libiridtools - library for the Irid toolchain
   Copyright (c) 2023 bellrise */

#pragma once

#include <stddef.h>
#include <string>

typedef unsigned char byte;

template <typename T>
struct range
{
    const T *ptr;
    const size_t len;

    range(const T *ptr, size_t len)
        : ptr(ptr)
        , len(len)
    { }

    template <typename K>
    range(const K *ptr, size_t len)
        : ptr(reinterpret_cast<const T *>(ptr))
        , len(len)
    { }
};

template <typename T>
range<T> range_from_string(const std::string& str)
{
    return range<T>(reinterpret_cast<const T *>(str.c_str()), str.size());
}

/**
 * Class for storing a range of raw bytes, without any specific memory layout.
 */
class bytebuffer
{
  public:
    bytebuffer();
    bytebuffer(const bytebuffer&);
    bytebuffer(bytebuffer&&);
    ~bytebuffer();

    size_t len() const;
    void clear();

    /* Freezing the buffer makes the pointer from get_range safe to use. */
    void freeze();
    void unfreeze();

    void append(byte);
    void append_range(range<byte>);
    void append_buffer(const bytebuffer&);
    void insert(byte, size_t index);
    void insert_range(range<byte>, size_t starting_index);
    void insert_fill(byte with_byte, size_t starting_index, size_t len);

    template <typename T>
    void append_range(const T *ptr, size_t size)
    {
        range<byte> r = {reinterpret_cast<const byte *>(ptr), size};
        append_range(r);
    }

    byte at(size_t index) const;
    range<byte> get_range(size_t starting_index, size_t len) const;

    bytebuffer& operator=(const bytebuffer&);

  private:
    byte *m_alloc;
    size_t m_size;
    size_t m_space;
    bool m_frozen;

    void check_freeze();
    byte *checked_new(size_t allocation_size);
    void ensure_size(size_t required_size);
};
