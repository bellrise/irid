/* ld.h - Irid linker
   Copyright (c) 2023 bellrise */

#pragma once

#include <libiridtools/bytebuffer.h>
#include <vector>

#define LD_VER_MAJOR 0
#define LD_VER_MINOR 1

struct options
{
    std::string output;
    std::vector<std::string> inputs;
};

void opt_parse(options& opts, int argc, char **argv);

struct linker
{
    linker();

    void add_object(const bytebuffer&);
    bytebuffer link();

  private:
};

void die(const char *fmt, ...);
