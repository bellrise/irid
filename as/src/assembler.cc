/* Irid assembler
  Copyright (c) 2023 bellrise */

#include "as.h"

#include <ctype.h>
#include <iostream>
#include <irid/arch.h>
#include <irid/iof.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

assembler::assembler(const std::string& inputname, const std::string& source)
    : m_inputname(inputname)
    , m_source(source)
{
    reset_state_variables();

    m_warnings.fill(true);
    register_directive_methods();
    register_instruction_methods();
}

assembler::~assembler() { }

bool assembler::assemble()
{
    std::vector<std::string> source_lines;
    source_line line;

    source_lines = split_lines(m_source);
    for (std::string& line : source_lines)
        line = rstrip_string(remove_comments(line));

    for (size_t i = 0; i < source_lines.size(); i++) {
        if (source_lines[i].empty())
            continue;

        /* There are 3 types of lines: labels, directives & instructions. */

        line = source_line(source_lines[i], i + 1);
        if (line.str.ends_with(':')) {
            parse_label(line);
        } else if (lstrip_string(line.str).starts_with('.')) {
            parse_directive(line);
        } else {
            parse_instruction(line);
        }
    }

    return false;
}

bytebuffer assembler::as_object()
{
    iof_builder builder;
    iof_section_builder section;
    bool found_label;

    section.set_code(m_code);
    section.set_name(m_inputname);
    section.set_flag(0);
    section.set_origin(0);

    for (const link_point& link_point : m_link_points)
        section.add_link(link_point.symbol, link_point.offset);
    for (const exported_name& exported_name : m_exports) {
        /* We need to find the exported name in the label array. */

        found_label = false;
        for (const label& label : m_labels) {
            if (label.name == exported_name.name) {
                section.add_export(label.name, label.offset);
                found_label = true;
                break;
            }
        }

        if (!found_label) {
            error(exported_name.decl_line, exported_name.line_offset,
                  "cannot find exported label");
        }
    }

    builder.add_section(section);

    return builder.build();
}

bytebuffer assembler::as_raw_binary()
{
    for (const link_point& link_point : m_link_points)
        link(link_point);

    return m_code;
}

void assembler::set_warning(warning_type warning, bool true_or_false)
{
    if (warning >= warning_type::_WARNING_TYPE_LEN)
        throw std::logic_error("warning type is out of range");
    m_warnings[static_cast<size_t>(warning)] = true_or_false;
}

void assembler::reset_state_variables()
{
    m_link_points.clear();
    m_last_label.clear();
    m_exports.clear();
    m_values.clear();
    m_labels.clear();
    m_code.clear();
    m_code.clear();
    m_pos = 0;
}

void assembler::register_directive_methods()
{
    m_directives.push_back(named_method("org", &assembler::directive_org));
    m_directives.push_back(
        named_method("string", &assembler::directive_string));
    m_directives.push_back(named_method("byte", &assembler::directive_byte));
    m_directives.push_back(named_method("resv", &assembler::directive_resv));
    m_directives.push_back(named_method("value", &assembler::directive_value));
    m_directives.push_back(
        named_method("export", &assembler::directive_export));
}

void assembler::register_instruction_methods()
{
    /* <no args> */
    m_instructions.push_back(
        named_method("cpucall", &assembler::ins_no_arguments));
    m_instructions.push_back(named_method("ret", &assembler::ins_no_arguments));
    m_instructions.push_back(named_method("rti", &assembler::ins_no_arguments));
    m_instructions.push_back(named_method("sti", &assembler::ins_no_arguments));
    m_instructions.push_back(named_method("dsi", &assembler::ins_no_arguments));

    /* rx, any */
    m_instructions.push_back(named_method("mov", &assembler::ins_dest_and_any));
    m_instructions.push_back(named_method("add", &assembler::ins_dest_and_any));
    m_instructions.push_back(named_method("sub", &assembler::ins_dest_and_any));
    m_instructions.push_back(named_method("cmp", &assembler::ins_dest_and_any));
    m_instructions.push_back(named_method("cmg", &assembler::ins_dest_and_any));
    m_instructions.push_back(named_method("cml", &assembler::ins_dest_and_any));
    m_instructions.push_back(named_method("and", &assembler::ins_dest_and_any));
    m_instructions.push_back(named_method("or", &assembler::ins_dest_and_any));

    /* rx */
    m_instructions.push_back(named_method("push", &assembler::ins_register));
    m_instructions.push_back(named_method("pop", &assembler::ins_register));

    /* rx, imm8 */
    m_instructions.push_back(named_method("shr", &assembler::ins_dest_and_r8));
    m_instructions.push_back(named_method("shl", &assembler::ins_dest_and_r8));

    /* rx, addr */
    m_instructions.push_back(
        named_method("jnz", &assembler::ins_dest_and_addr));

    /* rx, [addr/rx] */
    m_instructions.push_back(named_method("load", &assembler::ins_load));
    m_instructions.push_back(named_method("store", &assembler::ins_store));

    /* addr */
    m_instructions.push_back(named_method("jmp", &assembler::ins_addr));
    m_instructions.push_back(named_method("jeq", &assembler::ins_addr));
    m_instructions.push_back(named_method("call", &assembler::ins_addr));
}

bool assembler::warning_is_enabled(warning_type warning)
{
    if (warning >= warning_type::_WARNING_TYPE_LEN)
        throw std::logic_error("warning type is out of range");
    return m_warnings[static_cast<size_t>(warning)];
}

void assembler::insert_instruction(std::initializer_list<byte> bytes)
{
    byte instruction_bytes[4] = {static_cast<byte>(0)};
    auto byterange = range<byte>(instruction_bytes, 4);
    size_t align_offset;
    size_t index;

    /* All instructions must be 4-byte aligned, which creates a possibility of a
       label being misaligned with the proceeding instruction. In this case, we
       have to check the previous label and possibly align it with a 4 byte
       boundry, where the instruction will be placed. */

    index = 0;
    for (byte byte : bytes)
        instruction_bytes[index++] = byte;

    if (m_pos % 4 == 0) {
        m_code.insert_range(byterange, m_pos);
        m_pos += 4;
        return;
    }

    align_offset = 4 - (m_pos % 4);

    for (label& label : m_labels) {
        if (label.offset == m_pos) {
            label.offset += align_offset;
            break;
        }
    }

    m_pos += align_offset;
    m_code.insert_range(byterange, m_pos);
    m_pos += 4;
}

void assembler::add_link_point(source_line& decl_line,
                               const std::string& symbol, size_t code_offset,
                               int line_offset)
{
    std::string resolved_symbol = symbol;

    if (resolved_symbol[0] == '@') {
        if (m_last_label.empty())
            error(decl_line, 0, "local label needs a regular label before it");
        resolved_symbol = m_last_label + resolved_symbol;
    }

    m_link_points.push_back(
        link_point(resolved_symbol, code_offset, decl_line, line_offset));
}

int assembler::resolve_address_or_link(source_line& line,
                                       const std::string& symbol,
                                       size_t code_offset, int line_offset)
{
    for (const auto& val : m_values) {
        if (val.name == symbol)
            return val.value;
    }

    add_link_point(line, symbol, code_offset, line_offset);
    return 0;
}

void assembler::link(const link_point& point)
{
    for (const label& lbl : m_labels) {
        if (lbl.name == point.symbol) {
            m_code.insert(byte(lbl.offset % 256), point.offset);
            m_code.insert(byte(lbl.offset >> 8), point.offset + 1);
            return;
        }
    }

    error(point.decl_line, point.line_offset, "cannot find label `%s`",
          point.symbol.c_str());
}

static bool is_valid_symbol_char(char c)
{
    return isalnum(c) || c == '_' || c == '.';
}

static bool is_valid_start_symbol_char(char c)
{
    return isalpha(c) || c == '_' || c == '.';
}

static bool is_valid_symbol(const std::string& str)
{
    if (str[0] != '@' && !is_valid_start_symbol_char(str[0]))
        return false;

    if (str.size() == 1 && is_valid_start_symbol_char(str[0]))
        return true;

    for (char c : str.substr(1)) {
        if (!is_valid_symbol_char(c))
            return false;
    }

    return true;
}

void assembler::parse_label(source_line& line)
{
    bool append_nonlocal_label;
    std::string label_str;
    size_t error_offset;

    label_str = lstrip_string(line.str);
    error_offset = line.str.size() - label_str.size();
    label_str = label_str.substr(0, label_str.size() - 1);
    append_nonlocal_label = false;

    /* Validate the label. */

    if (!label_str.size())
        error(line, 0, "missing actual label");

    if (label_str[0] == '@') {
        if (m_last_label.empty()) {
            error(line, error_offset,
                  "regular label has to exist before local label");
        }

        append_nonlocal_label = true;
    } else if (!isalpha(label_str[0])) {
        error(line, error_offset,
              "invalid label, first char has to fulfill A-z");
    }

    for (size_t i = 1; i < label_str.size(); i++) {
        if (!is_valid_symbol_char(label_str[i])) {
            error(line, i + error_offset, "invalid character '%c' in label",
                  label_str[i]);
        }
    }

    for (const label& lab : m_labels) {
        if (lab.name != label_str)
            continue;
        error(line, error_offset, "this label was already declared on line %d",
              lab.declaration_line);
    }

    if (append_nonlocal_label)
        label_str = m_last_label + label_str;
    else
        m_last_label = label_str;

    m_labels.push_back(label(label_str, m_pos, line.line_number));
}

void assembler::parse_instruction(source_line& line)
{
    std::string mnemonic;

    mnemonic = line.parts[0];
    for (const named_method& method : m_instructions) {
        if (method.name != mnemonic)
            continue;
        method.method(*this, line);
        return;
    }

    error(line, line.part_offsets[0], "unknown instruction");
}

void assembler::parse_directive(source_line& line)
{
    std::string directive;

    directive = line.parts[0].substr(1);
    for (const named_method& dir : m_directives) {
        if (dir.name != directive)
            continue;
        dir.method(*this, line);
        return;
    }

    error(line, 0, "unknown directive");
}

void assembler::directive_org(source_line& line)
{
    size_t addr;

    if (line.parts.size() < 2)
        error(line, line.str.size(), "missing origin address");

    addr = parse_int(line.parts[1], line, line.part_offsets[1]);
    if (m_pos > addr) {
        warn(warning_type::OVERLAPING_ORG, line, line.part_offsets[1],
             "origin point may overlap existing data");
    }

    m_pos = addr;
}

void assembler::directive_string(source_line& line)
{
    std::string value;

    if (line.parts.size() < 2)
        error(line, line.str.size(), "missing string value");
    if (line.parts.size() > 2)
        error(line, line.part_offsets[2], "too many arguments");

    value = parse_string(line.parts[1], line, line.part_offsets[1]);
    auto range = range_from_string<byte>(value);

    m_code.insert_range(range, m_pos);
    m_pos += value.size();
    m_code.insert(0, m_pos++);
}

void assembler::directive_byte(source_line& line)
{
    int value;

    if (line.parts.size() < 2)
        error(line, line.parts[0].size(), "expected a byte value here");
    if (line.parts.size() > 2)
        error(line, line.part_offsets[2], "too many arguments");

    value = parse_int(line.parts[1], line, line.part_offsets[1]);
    if (value > 255) {
        error(line, line.part_offsets[1],
              "value for a byte is too large, try .word instead");
    }

    m_code.insert(static_cast<byte>(value), m_pos++);
}

void assembler::directive_resv(source_line& line)
{
    int bytes_to_resv;

    if (line.parts.size() < 2) {
        error(line, line.parts[0].size(),
              "expected an amount of bytes to reserve");
    }

    if (line.parts.size() > 2)
        error(line, line.part_offsets[2], "too many arguments");

    bytes_to_resv = parse_int(line.parts[1], line, line.part_offsets[1]);
    m_code.insert_fill(0, m_pos, bytes_to_resv);
    m_pos += bytes_to_resv;
}

void assembler::directive_value(source_line& line)
{
    std::string name;
    int value;

    if (line.parts.size() < 3)
        error(line, line.str.size(), "expected a value");
    if (line.parts.size() > 3)
        error(line, line.part_offsets[2], "too many arguments");

    name = line.parts[1];
    if (!is_valid_symbol(name))
        error(line, line.part_offsets[1], "this is not a valid symbol");

    value = parse_int(line.parts[2], line, line.part_offsets[2]);
    m_values.push_back({name, value});
}

void assembler::directive_export(source_line& line)
{
    if (line.parts.size() < 2)
        error(line, line.str.size(), "expected a name");
    if (line.parts.size() > 2)
        error(line, line.part_offsets[1], "too many arguments");

    if (!is_valid_symbol(line.parts[1])) {
        error(line, line.part_offsets[1],
              "exported symbol needs to be a valid name");
    }

    m_exports.push_back({line.parts[1], line, line.part_offsets[1]});
}

void assembler::directive_decl(source_line& line)
{
    if (line.parts.size() < 2)
        error(line, line.str.size(), "expected a name");
    if (line.parts.size() > 2)
        error(line, line.part_offsets[1], "too many arguments");

    if (!is_valid_symbol(line.parts[1])) {
        error(line, line.part_offsets[1],
              "exported symbol needs to be a valid name");
    }
}

std::vector<assembler::arg_variant>
assembler::parse_args(source_line& line,
                      std::initializer_list<arg_type> argument_types,
                      std::initializer_list<std::string> error_messages)
{
    std::vector<arg_variant> args;
    std::string missing_arg_str;
    arg_type argument_type;
    int int_value;

    if (argument_types.size() != error_messages.size()) {
        throw std::logic_error(
            "argument_types.size() != error_messages.size()");
    }

    if (line.parts.size() > argument_types.size()) {
        error(line, line.part_offsets[argument_types.size() - 1],
              "too many arguments");
    }

    for (size_t i = 0; i < argument_types.size(); i++) {
        argument_type = argument_types.begin()[i];

        if (line.parts.size() < i) {
            /* Missing argument. */
            missing_arg_str = "expected " + error_messages.begin()[i];
            error(line, line.str.size(), missing_arg_str.c_str());
        }

        if (argument_type == arg_type::REGISTER) {
            auto maybe_register = try_parse_register(line.parts[i + 1]);
            if (!maybe_register.has_value())
                error(line, line.part_offsets[i + 1], "expected register");

            args.push_back(byte(maybe_register.value()));
            continue;
        }

        /* Linkable symbol. */
        if (is_valid_symbol(line.parts[i + 1])) {
            if (argument_type == arg_type::IMM8) {
                error(line, line.part_offsets[i + 1],
                      "an 8-bit immediate is allowed here, an address is too "
                      "wide");
            }

            args.push_back(resolve_address_or_link(
                line, line.parts[i + 1], m_pos + 2, line.part_offsets[i + 1]));
            continue;
        }

        int_value =
            parse_int(line.parts[i + 1], line, line.part_offsets[i + 1]);

        if (argument_type == arg_type::IMM8) {
            if (int_value > 255) {
                error(line, line.part_offsets[i + 1],
                      "%d will not fit in an 8-bit value", int_value);
            }

            args.push_back(byte(int_value));
        } else {
            args.push_back(int_value);
        }
    }

    if (argument_types.size() != args.size())
        throw std::runtime_error("failed to ensure args");
    return args;
}

void assembler::ins_no_arguments(source_line& line)
{
    if (line.parts.size() > 1) {
        error(line, line.part_offsets[1], "%s does not take any arguments",
              line.parts[0].c_str());
    }

    insert_instruction({instruction_id_from_mnemonic(line.parts[0])});
}

void assembler::ins_dest_and_any(source_line& line)
{
    constexpr int IMM8_MODE = 1;
    constexpr int IMM16_MODE = 2;

    byte dest_register_byte;
    byte instruction_byte;
    std::string dest;
    std::string source;
    int instruction_mode;
    int immediate;
    int addr;

    if (line.parts.size() == 1)
        error(line, line.parts[0].size(), "missing destination register");
    if (line.parts.size() == 2) {
        error(line, line.part_offsets[1] + line.parts[1].size(),
              "missing source register or immediate");
    }

    if (line.parts.size() > 3) {
        error(line, line.part_offsets[3],
              "unwanted argument for %s instruction", line.parts[0].c_str());
    }

    dest = line.parts[1];
    source = line.parts[2];

    instruction_byte = instruction_id_from_mnemonic(line.parts[0]);
    if (instruction_byte == 0)
        throw std::runtime_error("could not get instruction ID from mnemonic");

    /* The destination, always a register. */

    auto maybe_dest_register = try_parse_register(dest);
    if (!maybe_dest_register.has_value()) {
        error(line, line.part_offsets[1],
              "expected a register as the destination argument");
    }

    dest_register_byte = maybe_dest_register.value();

    /* The source, a register or immediate. */

    auto maybe_source_register = try_parse_register(source);
    if (maybe_source_register.has_value()) {
        /* register, register */
        return insert_instruction(
            {instruction_byte, dest_register_byte,
             static_cast<byte>(maybe_source_register.value())});
    }

    if (is_valid_symbol(source)) {
        addr = resolve_address_or_link(line, source, m_pos + 2,
                                       line.part_offsets[2]);
        return insert_instruction({byte(instruction_byte + IMM16_MODE),
                                   dest_register_byte, byte(addr % 256),
                                   byte(addr >> 8)});
    }

    immediate = parse_int(source, line, line.part_offsets[2]);
    instruction_mode = immediate < 256 ? IMM8_MODE : IMM16_MODE;
    instruction_byte += instruction_mode;

    if (instruction_mode == IMM16_MODE
        && get_register_width(dest_register_byte) == register_width::BYTE) {
        warn(warning_type::OVERLAPING_ORG, line, line.part_offsets[2],
             "value does not fit in half-register, it will be truncated");
    }

    if (instruction_mode == IMM8_MODE) {
        /* register, imm8 */
        return insert_instruction(
            {instruction_byte, dest_register_byte, byte(immediate % 256)});
    }

    /* register, imm16 */
    return insert_instruction({instruction_byte, dest_register_byte,
                               byte(immediate % 256), byte(immediate >> 8)});
}

void assembler::ins_dest_and_r8(source_line& line)
{
    byte instruction_byte;
    int value;

    if (line.parts.size() == 1)
        error(line, line.parts[0].size(), "missing destination register");
    if (line.parts.size() == 2) {
        error(line, line.part_offsets[1] + line.parts[1].size(),
              "missing source register or immediate");
    }

    instruction_byte = instruction_id_from_mnemonic(line.parts[0]);
    if (instruction_byte == 0)
        throw std::runtime_error("could not get instruction ID from mnemonic");

    auto maybe_register = try_parse_register(line.parts[1]);
    if (!maybe_register.has_value()) {
        error(line, line.part_offsets[1],
              "expected a register as the destination argument");
    }

    auto maybe_source_register = try_parse_register(line.parts[2]);
    if (maybe_source_register.has_value()) {
        return insert_instruction({instruction_byte,
                                   byte(maybe_register.value()),
                                   byte(maybe_source_register.value())});
    }

    value = parse_int(line.parts[2], line, line.part_offsets[2]);
    if (value > 255)
        error(line, line.part_offsets[2], "value cannot fit in a 8-bit byte");

    return insert_instruction({byte(instruction_byte + 1),
                               byte(maybe_register.value()),
                               byte(value % 256)});
}

void assembler::ins_dest_and_addr(source_line& line)
{
    byte instruction_byte;
    std::string dest_str;
    std::string addr_str;
    int addr;

    if (line.parts.size() == 1)
        error(line, line.parts[0].size(), "missing destination register");
    if (line.parts.size() == 2) {
        error(line, line.part_offsets[1] + line.parts[1].size(),
              "missing address");
    }

    if (line.parts.size() > 3) {
        error(line, line.part_offsets[3],
              "unwanted argument for %s instruction", line.parts[0].c_str());
    }

    dest_str = line.parts[1];
    addr_str = line.parts[2];
    instruction_byte = instruction_id_from_mnemonic(line.parts[0]);

    auto maybe_register = try_parse_register(dest_str);
    if (!maybe_register.has_value())
        error(line, line.part_offsets[1], "expected a register");

    if (is_valid_symbol(addr_str)) {
        addr = resolve_address_or_link(line, addr_str, m_pos + 2,
                                       line.part_offsets[2]);
    } else {
        addr = parse_int(addr_str, line, line.part_offsets[1]);
    }

    return insert_instruction({instruction_byte, byte(maybe_register.value()),
                               byte(addr % 256), byte(addr >> 8)});
}

void assembler::ins_addr(source_line& line)
{
    std::string addr_str;
    byte instruction_byte;
    int addr;

    if (line.parts.size() < 2)
        error(line, line.str.size(), "missing address argument");

    addr_str = line.parts[1];
    instruction_byte = instruction_id_from_mnemonic(line.parts[0]);

    if (is_valid_symbol(addr_str)) {
        addr = resolve_address_or_link(line, addr_str, m_pos + 1,
                                       line.part_offsets[1]);
    } else {
        addr = parse_int(addr_str, line, line.part_offsets[1]);
    }

    insert_instruction({instruction_byte, byte(addr % 256), byte(addr >> 8)});
}

void assembler::ins_register(source_line& line)
{
    byte instruction_byte;

    if (line.parts.size() < 2)
        error(line, line.str.size(), "missing register argument");

    auto maybe_register = try_parse_register(line.parts[1]);
    if (!maybe_register.has_value())
        error(line, line.part_offsets[1], "expected a register");

    instruction_byte = instruction_id_from_mnemonic(line.parts[0]);

    insert_instruction({instruction_byte, byte(maybe_register.value())});
}

void assembler::ins_load(source_line& line)
{
    ins_load_and_store(line, I_LOAD, I_LOAD16);
}

void assembler::ins_store(source_line& line)
{
    ins_load_and_store(line, I_STORE, I_STORE16);
}

void assembler::ins_load_and_store(source_line& line, byte r_instruction,
                                   byte imm16_instruction)
{
    std::string addr_str;
    byte operand;
    int addr;

    if (line.parts.size() < 2)
        error(line, line.str.size(), "missing operand argument");
    if (line.parts.size() < 3)
        error(line, line.str.size(), "missing address argument");

    auto maybe_register = try_parse_register(line.parts[1]);
    if (!maybe_register.has_value())
        error(line, line.part_offsets[1], "expected a register");

    operand = maybe_register.value();
    addr_str = line.parts[2];

    auto maybe_addr = try_parse_register(line.parts[2]);
    if (maybe_addr.has_value()) {
        return insert_instruction(
            {r_instruction, operand, byte(maybe_addr.value())});
    }

    if (is_valid_symbol(addr_str)) {
        addr = resolve_address_or_link(line, addr_str, m_pos + 1,
                                       line.part_offsets[2]);
    } else {
        addr = parse_int(addr_str, line, line.part_offsets[1]);
    }

    return insert_instruction(
        {imm16_instruction, operand, byte(addr % 256), byte(addr >> 8)});
}

void assembler::error(const source_line& line, int position_in_line,
                      const char *fmt, ...)
{
    std::string code_snippet;
    va_list args;
    va_start(args, fmt);

    code_snippet = line.str;
    replace_char_and_count(code_snippet, '\t', ' ');

    fprintf(stderr, "irid-as: \033[31merror\033[0m in %s\n",
            m_inputname.c_str());
    fprintf(stderr, "     |\n");
    fprintf(stderr, "% 4d | %s\n", line.line_number, code_snippet.c_str());
    fprintf(stderr, "     | ");

    for (int i = 0; i < position_in_line; i++)
        fputc(' ', stderr);

    fprintf(stderr, "\033[1;31m^ ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n\n");

    va_end(args);
    exit(1);
}

void assembler::warn(warning_type warning, const source_line& line,
                     int position_in_line, const char *fmt, ...)
{
    std::string code_snippet;
    va_list args;
    va_start(args, fmt);

    /* Check if this warning is enabled first. */
    if (!warning_is_enabled(warning)) {
        va_end(args);
        return;
    }

    code_snippet = line.str;
    replace_char_and_count(code_snippet, '\t', ' ');

    fprintf(stderr, "irid-as: \033[33mwarning\033[0m in %s\n",
            m_inputname.c_str());
    fprintf(stderr, "    |\n");
    fprintf(stderr, "% 3d | %s\n", line.line_number, code_snippet.c_str());
    fprintf(stderr, "    | ");

    for (int i = 0; i < position_in_line; i++)
        fputc(' ', stderr);

    fprintf(stderr, "\033[1;33m^ ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n\n");

    va_end(args);
}

static int escape_char_to_byte(int c)
{
    switch (c) {
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'e':
        return '\e';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    case '\\':
        return '\\';
    case '"':
        return '"';
    case '\'':
        return '\'';
    default:
        return 0;
    }
}

int assembler::parse_int(const std::string& source, const source_line& src,
                         int pos_in_line)
{
    long parsed_value;
    char *end_ptr;

    if (source.starts_with('\'')) {
        if (!source.ends_with('\'')) {
            error(src, pos_in_line + source.size(),
                  "character value is missing a quote at the end");
        }

        if (source.size() < 3)
            error(src, pos_in_line, "character value is missing a value");

        if (source[1] == '\\') {
            /* Escaped char */
            if (source.size() != 4) {
                error(src, pos_in_line + 1,
                      "there can only be one byte in a character");
            }

            return escape_char_to_byte(source[2]);
        } else {
            /* Regular ASCII char. */
            if (source.size() != 3) {
                error(src, pos_in_line + 1,
                      "there can only be one byte in a character");
            }

            return source[1];
        }
    }

    parsed_value = strtol(source.c_str(), &end_ptr, 0);
    if (parsed_value == LONG_MAX || parsed_value > IRID_MAX_ADDR)
        error(src, pos_in_line, "value is above the addressable range");
    if (parsed_value == LONG_MIN)
        error(src, pos_in_line, "value is below the allowed range");

    if (source.c_str() == end_ptr)
        error(src, pos_in_line, "expected integer value");

    return parsed_value;
}

std::string assembler::parse_string(std::string str, const source_line& src,
                                    int pos_in_line)
{
    std::string result;

    if (!str.starts_with('"'))
        error(src, pos_in_line, "expected string");
    if (!str.ends_with('"')) {
        error(src, str.size() + pos_in_line,
              "expected the string to end with a double-quote (\")");
    }

    str = str.substr(1, str.size() - 2);

    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '\\') {
            if (i == str.size() - 1) {
                error(src, pos_in_line + i + 1,
                      "expected another character after the backslash (\\)");
            }

            result.push_back(escape_char_to_byte(str[++i]));
            continue;
        }

        result.push_back(str[i]);
    }

    return result;
}

std::optional<int> assembler::try_parse_register(const std::string& reg)
{
    if (reg.size() != 2)
        return {};

    /* r[0-7] */
    if (reg[0] == 'r') {
        if (reg[1] < '0' || reg[1] > '7')
            return {};
        return R_R0 + (reg[1] - '0');
    }

    /* h[0-3] or l[0-3] */
    if (reg[0] == 'h' || reg[0] == 'l') {
        if (reg[1] < '0' || reg[1] > '3')
            return {};
        return (reg[0] == 'h' ? R_H0 : R_L0) + (reg[1] - '0');
    }

    if (reg == "ip")
        return R_IP;
    if (reg == "sp")
        return R_SP;
    if (reg == "bp")
        return R_BP;

    return {};
}

byte assembler::instruction_id_from_mnemonic(const std::string& mnemonic)
{
    static constexpr std::pair<std::string, byte> mnemonic_map[] = {
        {"cpucall", I_CPUCALL}, {"call", I_CALL}, {"ret", I_RET},
        {"rti", I_RTI},         {"sti", I_STI},   {"dsi", I_DSI},
        {"mov", I_MOV},         {"jmp", I_JMP},   {"jeq", I_JEQ},
        {"add", I_ADD},         {"sub", I_SUB},   {"jnz", I_JNZ},
        {"push", I_PUSH},       {"pop", I_POP},   {"load", I_LOAD},
        {"store", I_STORE},     {"cmg", I_CMG},   {"cml", I_CML},
        {"cmp", I_CMP},         {"and", I_AND},   {"or", I_OR},
        {"shr", I_SHR},         {"shl", I_SHL}};
    static const size_t map_size =
        sizeof(mnemonic_map) / sizeof(std::pair<std::string, int>);

    for (size_t i = 0; i < map_size; i++) {
        if (mnemonic_map[i].first == mnemonic)
            return mnemonic_map[i].second;
    }

    return 0;
}

assembler::register_width assembler::get_register_width(byte register_id)
{
    if (int(register_id) >= R_H0 && int(register_id) <= R_L3)
        return register_width::BYTE;
    return register_width::WORD;
}

std::optional<assembler::label> assembler::find_label(const std::string& name)
{
    for (label& lbl : m_labels) {
        if (lbl.name == name)
            return lbl;
    }

    return {};
}

std::vector<std::string>
assembler::split_string_parts(const std::string& line,
                              std::vector<int>& part_offsets)
{
    std::vector<std::string> parts;
    std::string buffered_part;
    size_t start_of_part;
    char in_string;

    in_string = 0;
    start_of_part = 0;

    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == '"' && !in_string)
            in_string = '"';
        else if (line[i] == '"' && in_string)
            in_string = 0;
        else if (line[i] == '\'' && !in_string)
            in_string = '\'';
        else if (line[i] == '\'' && in_string)
            in_string = 0;

        if (!in_string && isspace(line[i]) && buffered_part.empty())
            continue;

        if (!in_string && isspace(line[i]) && buffered_part.size()) {
            if (buffered_part.ends_with(',')) {
                buffered_part =
                    buffered_part.substr(0, buffered_part.size() - 1);
            }

            parts.push_back(buffered_part);
            part_offsets.push_back(start_of_part);
            buffered_part.clear();
            continue;
        }

        if (buffered_part.empty())
            start_of_part = i;
        buffered_part += line[i];
    }

    parts.push_back(buffered_part);
    part_offsets.push_back(start_of_part);

    return parts;
}

std::vector<std::string> assembler::split_lines(const std::string& source)
{
    std::vector<std::string> lines;
    std::string buffer_line;
    size_t start_index;

    start_index = 0;

    for (size_t i = 0; i < source.size(); i++) {
        if (source[i] != '\n')
            continue;

        lines.push_back(source.substr(start_index, i - start_index));
        start_index = i + 1;
    }

    lines.push_back(source.substr(start_index));

    return lines;
}

std::string assembler::lstrip_string(const std::string& str)
{
    for (size_t i = 0; i < str.size(); i++) {
        if (!isspace(str[i]))
            return str.substr(i);
    }

    return str;
}

std::string assembler::rstrip_string(const std::string& str)
{
    for (size_t i = str.size() - 1; i > 0; i--) {
        if (!isspace(str[i]))
            return str.substr(0, i + 1);
    }

    return str;
}

std::string assembler::strip_string(const std::string& str)
{
    return lstrip_string(rstrip_string(str));
}

std::string assembler::remove_comments(const std::string& line)
{
    char in_string;

    if (strip_string(line).starts_with(';'))
        return "";

    in_string = 0;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == '"' && !in_string)
            in_string = '"';
        else if (line[i] == '"' && in_string)
            in_string = 0;

        if (line[i] == '\'' && !in_string)
            in_string = '\'';
        else if (line[i] == '\'' && in_string)
            in_string = 0;

        if (!in_string && line[i] == ';')
            return line.substr(0, i);
    }

    return line;
}

int assembler::replace_char_and_count(std::string& str, char from, char to)
{
    int replaced = 0;

    for (char& c : str) {
        if (c == from) {
            c = to;
            replaced++;
        }
    }

    return replaced;
}
