/* Irid assembler
   Copyright (c) 2023 bellrise */

#include "as.h"

#include <fstream>
#include <sstream>

static std::string read_input(const std::string& path);

int main(int argc, char **argv)
{
    options opts;

    opt_set_defaults(opts);
    opt_parse(opts, argc, argv);

    assembler as(opts.input, read_input(opts.input));

    as.assemble();
}

std::string read_input(const std::string& path)
{
    std::string content;
    char stdin_buffer[512];
    size_t read_chars;

    if (path == "-") {
        do {
            read_chars = std::fread(stdin_buffer, 1, 512, stdin);
            if (!read_chars)
                break;
            content.append(stdin_buffer, read_chars);
        } while (1);

        return content;
    }

    std::ifstream source_file;
    std::stringstream content_stream;

    source_file.open(path);
    if (!source_file.is_open())
        die("source file `%s` could not be opened", path.c_str());

    content_stream << source_file.rdbuf();
    return content_stream.str();
}
