/* as.h - assembler for Irid assembly
   Copyright (c) 2023 bellrise */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#define AS_VER_MAJOR 0
#define AS_VER_MINOR 1

struct options
{
    std::string input;
    std::string output;
};

void opt_set_defaults(options&);
void opt_parse(options&, int argc, char **argv);

template <typename T>
struct range
{
    const T *ptr;
    const size_t len;

    range(T *ptr, size_t len)
        : ptr(ptr)
        , len(len)
    { }
};

/**
 * Class for storing a range of raw bytes, without any specific memory layout.
 */
class bytebuffer
{
  public:
    bytebuffer();
    ~bytebuffer();

    size_t len() const;

    void append(std::byte);
    void append_range(range<std::byte>&);
    void insert(std::byte, size_t index);
    void insert_range(range<std::byte>&, size_t starting_index);

    std::byte at(size_t index) const;
    range<std::byte> get_range(size_t starting_index, size_t len) const;

  private:
    std::byte *m_alloc;
    size_t m_size;
    size_t m_space;

    std::byte *checked_new(size_t allocation_size);
    void ensure_size(size_t required_size);
};

struct iof_object
{
    const bytebuffer raw_bytes;
};

/**
 * Public assembler API. Setup an assembler with a source and source file name,
 * and call .assemble() on it. Later retrieve the generated binary using either
 * .as_object() or .as_binary().
 */
class assembler
{
  public:
    assembler(const std::string& inputname, const std::string& source);
    ~assembler();

    /* Returns true on successful assembly. */
    bool assemble();

    /* Retrieve a linkable object in the IOF format. */
    iof_object as_object();

    /* Retrieve a raw binary format of the processed code, directly executable
       on the CPU. */
    bytebuffer as_binary();

  private:
    struct source_line
    {
        std::string str;
        int line_number;
    };

    struct label
    {
        std::string name;
        size_t offset;
        int declaration_line;
    };

    /* General variables. */
    std::string m_inputname;
    std::string m_source;
    bytebuffer m_result;

    /* State variables. */
    std::vector<label> m_labels;
    size_t m_pos;

    void reset_state_variables();

    std::vector<std::string> split_lines(const std::string& source);
    std::string lstrip_string(const std::string&);
    std::string rstrip_string(const std::string&);
    std::string strip_string(const std::string&);
    std::string remove_comments(const std::string& line);

    void parse_label(source_line&);
    void parse_instruction(source_line&);
    void parse_directive(source_line&);

    void error(source_line&, int position_in_line, const char *fmt, ...);
};

void die(const char *fmt, ...);
