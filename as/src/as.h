/* as.h - assembler for Irid assembly
   Copyright (c) 2023 bellrise */

#pragma once

#include <cstddef>
#include <functional>
#include <irid/iof.h>
#include <libiridtools/bytebuffer.h>
#include <libiridtools/iof_builder.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#define AS_VER_MAJOR 0
#define AS_VER_MINOR 7

constexpr size_t INVALID_ORIGIN = -1;

struct options
{
    std::string input;
    std::string output;
    bool warn_origin_overlap;
    bool warn_comma_after_arg;
    bool raw_binary;
};

class assembler;

void opt_set_defaults(options&);
void opt_set_warnings_for_as(assembler&, options&);
void opt_parse(options&, int argc, char **argv);

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

    struct exported_name
    {
        std::string name;
        source_line decl_line;
        int line_offset;
    };

    struct named_method
    {
        std::string name;
        std::function<void(assembler&, source_line&)> method;
    };

    struct named_value
    {
        std::string name;
        int value;
    };

    struct link_point
    {
        std::string symbol;
        size_t offset;
        source_line decl_line;
        int line_offset;
    };

    enum class register_width
    {
        BYTE,
        WORD
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
    std::vector<link_point> m_link_points;
    std::vector<exported_name> m_exports;
    std::vector<named_value> m_values;
    std::vector<label> m_labels;
    std::string m_last_label;
    bytebuffer m_code;
    size_t m_origin;
    size_t m_pos;

    void reset_state_variables();
    void register_directive_methods();
    void register_instruction_methods();
    bool warning_is_enabled(warning_type warning);

    void insert_instruction(std::initializer_list<byte> bytes);
    void add_link_point(source_line&, const std::string& symbol,
                        size_t code_offset, int line_offset);

    int resolve_address_or_link(source_line&, const std::string& symbol,
                                size_t code_offset, int line_offset);

    void link(const link_point&);

    void parse_label(source_line&);
    void parse_instruction(source_line&);
    void parse_directive(source_line&);

    void directive_org(source_line&);
    void directive_string(source_line&);
    void directive_byte(source_line&);
    void directive_resv(source_line&);
    void directive_value(source_line&);
    void directive_export(source_line&);
    void directive_decl(source_line&);

    enum class arg_type
    {
        REGISTER,
        IMM8,
        IMM16,
    };

    using arg_variant = std::variant<byte, int>;

    std::vector<arg_variant>
    parse_args(source_line&, std::initializer_list<arg_type> argument_types,
               std::initializer_list<std::string> error_messages);

    /* Instruction with no arguments. */
    void ins_no_arguments(source_line&);

    /* Instruction with a destination register and a source register or
       immediate. */
    void ins_dest_and_any(source_line&);

    /* Instruction with a destination register, and a source register or
       immediate (only 8-bit). */
    void ins_dest_and_r8(source_line&);

    /* Instruction with a destination register and an address. */
    void ins_dest_and_addr(source_line&);

    /* Instruction with a single address. */
    void ins_addr(source_line&);

    /* Instruction with a single register. */
    void ins_register(source_line&);

    /* Load & store instructions. */
    void ins_load(source_line&);
    void ins_store(source_line&);

    void ins_load_and_store(source_line&, byte r_instruction,
                            byte imm16_instruction);

    void error(const source_line&, int position_in_line, const char *fmt, ...);
    void warn(warning_type warning, const source_line&, int position_in_line,
              const char *fmt, ...);

    int parse_int(const std::string& any_int_representation,
                  const source_line& src, int pos_in_line);
    std::string parse_string(std::string any_string_representation,
                             const source_line& src, int pos_in_line);
    std::optional<int>
    try_parse_register(const std::string& register_representation);
    byte instruction_id_from_mnemonic(const std::string& mnemonic);
    register_width get_register_width(byte register_id);

    std::optional<label> find_label(const std::string& name);

    static std::vector<std::string>
    split_string_parts(const std::string&, std::vector<int>& part_offsets);
    static std::vector<std::string> split_lines(const std::string& source);
    static std::string lstrip_string(const std::string&);
    static std::string rstrip_string(const std::string&);
    static std::string strip_string(const std::string&);
    static std::string remove_comments(const std::string& line);
    static int replace_char_and_count(std::string& str, char from, char to);
};

void die(const char *fmt, ...);
