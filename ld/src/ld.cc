/* irid-ld - irid linker
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <fstream>

static bytebuffer load_file(const std::string& file)
{
    std::ifstream f;
    bytebuffer buf;
    byte line[1024];

    f.open(file, std::ios::in | std::ios::binary);
    if (!f.is_open())
        die("failed to open file %s", file.c_str());

    while (1) {
        f.read((char *) line, 1024);
        if (!f.gcount())
            break;
        buf.append_range(line, f.gcount());
    }

    return buf;
}

static void write_file(const std::string& file, const bytebuffer& buf)
{
    std::ofstream f;

    f.open(file);
    if (!f.is_open())
        die("failed to open output file %s", file.c_str());

    auto r = buf.get_range(0, buf.len());
    f.write((char *) r.ptr, r.len);
}

int main(int argc, char **argv)
{
    options opts;
    linker link;

    opt_parse(opts, argc, argv);

    for (const std::string& input : opts.inputs)
        link.add_object(load_file(input));

    write_file(opts.output, link.link());
}
