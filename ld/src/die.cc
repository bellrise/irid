/* Irid linker
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <stdarg.h>
#include <stdio.h>

void debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-ld: \033[1;39m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);
}

void die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-ld: \033[1;31merror: \033[1;39m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);

    exit(1);
}
