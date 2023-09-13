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
    return bytebuffer();
}

bytebuffer assembler::as_raw_binary()
{
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
    m_labels.clear();
    m_code.clear();
    m_pos = 0;
}

void assembler::register_directive_methods()
{
    m_directives.push_back(named_method("org", &assembler::directive_org));
    m_directives.push_back(
        named_method("string", &assembler::directive_string));
    m_directives.push_back(named_method("byte", &assembler::directive_byte));
}

void assembler::register_instruction_methods()
{
    m_instructions.push_back(
        named_method("cpucall", &assembler::ins_no_arguments));
    m_instructions.push_back(named_method("ret", &assembler::ins_no_arguments));
    m_instructions.push_back(named_method("rti", &assembler::ins_no_arguments));
    m_instructions.push_back(named_method("sti", &assembler::ins_no_arguments));
    m_instructions.push_back(named_method("dsi", &assembler::ins_no_arguments));
}

bool assembler::warning_is_enabled(warning_type warning)
{
    if (warning >= warning_type::_WARNING_TYPE_LEN)
        throw std::logic_error("warning type is out of range");
    return m_warnings[static_cast<size_t>(warning)];
}

void assembler::insert_instruction(std::initializer_list<int> bytes)
{
    std::byte instruction_bytes[4] = {static_cast<std::byte>(0)};
    auto byterange = range<std::byte>(instruction_bytes, 4);
    size_t align_offset;
    size_t index;

    /* All instructions must be 4-byte aligned, which creates a possibility of a
       label being misaligned with the proceeding instruction. In this case, we
       have to check the previous label and possibly align it with a 4 byte
       boundry, where the instruction will be placed. */

    index = 0;
    for (int byte : bytes)
        instruction_bytes[index++] = static_cast<std::byte>(byte);

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

static bool is_valid_symbol_char(char c)
{
    return isalnum(c) || c == '_' || c == '.';
}

void assembler::parse_label(source_line& line)
{
    std::string label_str;
    size_t error_offset;

    label_str = lstrip_string(line.str);
    error_offset = line.str.size() - label_str.size();
    label_str = label_str.substr(0, label_str.size() - 1);

    /* Validate the label. */

    if (!label_str.size())
        error(line, 0, "missing actual label");

    if (!isalpha(label_str[0])) {
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
        break;
    }
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
    // TODO
}

void assembler::directive_byte(source_line& line)
{
    int value;

    if (line.parts.size() < 2)
        error(line, line.parts[0].size(), "expected a byte value here");
    if (line.parts.size() > 2)
        error(line, line.part_offsets[2], "too many arguments to directive");

    value = parse_int(line.parts[1], line, line.part_offsets[1]);
    if (value > 255) {
        error(line, line.part_offsets[1],
              "value for a byte is too large, try .word instead");
    }

    m_code.insert(static_cast<std::byte>(value), m_pos++);
}

void assembler::ins_no_arguments(source_line& line)
{
    if (line.parts.size() > 1) {
        error(line, line.part_offsets[1], "%s does not take any arguments",
              line.parts[0].c_str());
    }

    struct name_map_type
    {
        const std::string name;
        const int instruction_byte;
    };

    static constexpr name_map_type name_map[] = {
        {"cpucall", I_CPUCALL}, {"ret", I_RET}, {"rti", I_RTI},
        {"sti", I_STI},         {"dsi", I_DSI},
    };

    static constexpr size_t name_map_len =
        sizeof(name_map) / sizeof(name_map_type);

    for (size_t i = 0; i < name_map_len; i++) {
        if (line.parts[0] == name_map[i].name) {
            insert_instruction({name_map[i].instruction_byte});
            break;
        }
    }
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
    fprintf(stderr, "    |\n");
    fprintf(stderr, "% 3d | %s\n", line.line_number, code_snippet.c_str());
    fprintf(stderr, "    | ");

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

            switch (source[2]) {
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
            default:
                return 0;
            }

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
        error(src, pos_in_line, "value is above the allowed range");
    if (parsed_value == LONG_MIN)
        error(src, pos_in_line, "value is below the allowed range");

    if (source.c_str() == end_ptr)
        error(src, pos_in_line, "expected integer value");

    return parsed_value;
}

std::vector<std::string>
assembler::split_string_parts(const std::string& line,
                              std::vector<int>& part_offsets)
{
    std::vector<std::string> parts;
    std::string buffered_part;
    size_t start_index;
    size_t end_index;
    char in_string;

    in_string = 0;
    start_index = 0;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == '"' && !in_string)
            in_string = '"';
        else if (line[i] == '"' && in_string)
            in_string = 0;
        if (line[i] == '\'' && !in_string)
            in_string = '\'';
        else if (line[i] == '\'' && in_string)
            in_string = 0;

        if (in_string || !isspace(line[i]))
            continue;

        end_index = i;
        while (isspace(line[i]))
            i++;

        buffered_part = line.substr(start_index, end_index - start_index);
        start_index = i;

        if (!buffered_part.size())
            continue;

        if (buffered_part.ends_with(','))
            buffered_part = buffered_part.substr(0, buffered_part.size() - 1);

        part_offsets.push_back(start_index);
        parts.push_back(buffered_part);
    }

    part_offsets.push_back(start_index);
    parts.push_back(line.substr(start_index));

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
