/* Irid assembler
   Copyright (c) 2023 bellrise */

#include "as.h"

#include <fstream>
#include <sstream>

static std::string read_input(const std::string& path);
static void write_to_output(const bytebuffer& result, const std::string& path);

int main(int argc, char **argv)
{
    options opts;

    opt_set_defaults(opts);
    opt_parse(opts, argc, argv);

    assembler as(opts.input, read_input(opts.input));

    opt_set_warnings_for_as(as, opts);

    as.assemble();

    write_to_output(opts.raw_binary ? as.as_raw_binary() : as.as_object(),
                    opts.output);
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

void write_to_output(const bytebuffer& result, const std::string& path)
{
    std::ofstream dest_file;

    dest_file.open(path);
    if (!dest_file.is_open())
        die("destination file `%s` could not be opened", path.c_str());

    auto code_range = result.get_range(0, result.len());
    dest_file.write(reinterpret_cast<const char *>(code_range.ptr),
                    code_range.len);
}
