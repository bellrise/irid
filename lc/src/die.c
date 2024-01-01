/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void debug_impl(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-lc: \033[1;39m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);
}

void die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-lc: \033[1;31merror: \033[1;39m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);

    exit(1);
}
