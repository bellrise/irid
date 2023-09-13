/* Irid assembler
   Copyright (c) 2023 bellrise */

#include "as.h"

#include <ctype.h>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>

assembler::assembler(const std::string& inputname, const std::string& source)
    : m_inputname(inputname)
    , m_source(source)
{ }

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

iof_object assembler::as_object()
{
    return iof_object();
}

bytebuffer assembler::as_binary()
{
    return bytebuffer();
}

void assembler::reset_state_variables()
{
    m_labels.clear();
    m_pos = 0;
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
    // TODO
}

void assembler::parse_directive(source_line& line)
{
    // TODO
}

void assembler::error(source_line& line, int position_in_line, const char *fmt,
                      ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-as: \033[31merror\033[0m in %s\n",
            m_inputname.c_str());
    fprintf(stderr, "    |\n");
    fprintf(stderr, "% 3d | %s\n", line.line_number, line.str.c_str());
    fprintf(stderr, "    | ");

    for (int i = 0; i < position_in_line; i++)
        fputc(' ', stderr);

    fprintf(stderr, "\033[1;31m^ ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n\n");

    va_end(args);
    exit(1);
}
