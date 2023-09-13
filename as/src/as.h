/* as.h - assembler for Irid assembly
   Copyright (c) 2023 bellrise */

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#define AS_VER_MAJOR 0
#define AS_VER_MINOR 1

struct options
{
    std::string input;
    std::string output;
    bool warn_origin_overlap;
};

class assembler;

void opt_set_defaults(options&);
void opt_set_warnings_for_as(assembler&, options&);
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
    bytebuffer(const bytebuffer&);
    bytebuffer(bytebuffer&&);
    ~bytebuffer();

    size_t len() const;
    void clear();

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

enum struct warning_type
{
    OVERLAPING_ORG,
    _WARNING_TYPE_LEN
};

/**
 * Public assembler API. Setup an assembler with a source and source file name,
 * and call .assemble() on it. Later retrieve the generated IOF object using
 * .as_object().
 */
class assembler
{
  public:
    assembler(const std::string& inputname, const std::string& source);
    ~assembler();

    /* Returns true on successful assembly. */
    bool assemble();

    /* Retrieve a linkable object in the IOF format. */
    bytebuffer as_object();

    /* Retrieve raw binary code linked in-place. */
    bytebuffer as_raw_binary();

    /* Enable or disable warning. All warnings are enabled by default. */
    void set_warning(warning_type warning, bool true_or_false);

  private:
    struct source_line
    {
        std::string str;
        int line_number;
        std::vector<std::string> parts;
        std::vector<int> part_offsets;

        source_line() { }
        source_line(const std::string& str, int line_number)
            : str(str)
            , line_number(line_number)
        {
            parts = split_string_parts(str, part_offsets);
        }
    };

    struct label
    {
        std::string name;
        size_t offset;
        int declaration_line;
    };

    struct named_method
    {
        std::string name;
        std::function<void(assembler&, source_line&)> method;
    };

    static constexpr size_t m_warnings_len =
        static_cast<size_t>(warning_type::_WARNING_TYPE_LEN);

    /* General variables. */
    std::string m_inputname;
    std::string m_source;

    /* Groups */
    std::vector<named_method> m_directives;
    std::vector<named_method> m_instructions;
    std::array<bool, m_warnings_len> m_warnings;

    /* State variables. */
    std::vector<label> m_labels;
    bytebuffer m_code;
    size_t m_pos;

    void reset_state_variables();
    void register_directive_methods();
    void register_instruction_methods();
    bool warning_is_enabled(warning_type warning);

    void insert_instruction(std::initializer_list<int> bytes);

    void parse_label(source_line&);
    void parse_instruction(source_line&);
    void parse_directive(source_line&);

    void directive_org(source_line&);
    void directive_string(source_line&);
    void directive_byte(source_line&);

    void ins_no_arguments(source_line&);

    void error(const source_line&, int position_in_line, const char *fmt, ...);
    void warn(warning_type warning, const source_line&, int position_in_line,
              const char *fmt, ...);

    int parse_int(const std::string& any_int_representation,
                  const source_line& src, int pos_in_line);

    static std::vector<std::string>
    split_string_parts(const std::string&, std::vector<int>& part_offsets);
    static std::vector<std::string> split_lines(const std::string& source);
    static std::string lstrip_string(const std::string&);
    static std::string rstrip_string(const std::string&);
    static std::string strip_string(const std::string&);
    static std::string remove_comments(const std::string& line);
};

void die(const char *fmt, ...);
