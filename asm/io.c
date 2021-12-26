/* I/O routines
   Copyright (C) 2021 bellrise */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>


int die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-asm: \033[1;31merror: \033[1;39m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);

    exit(1);
    return 0;
}

int warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-asm: \033[1;35mwarning: \033[1;39m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);
    return 0;
}

int _info_internal(const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-asm: \033[1;36minfo: \033[0m(%s)\033[1;39m ", func);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);
    return 0;
}
