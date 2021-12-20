/* Output routines.
   Copyright (C) 2021 bellrise */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "emul.h"


int die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-emul: \033[1;31merror: \033[1;39m");
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

    fprintf(stderr, "irid-emul: \033[1;35mwarning: \033[1;39m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);
    return 0;
}

int _info_internal(const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "irid-emul: \033[1;36minfo: \033[0m(%s)\033[1;39m ", func);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);
    return 0;
}

void page_info_d(const struct page_info *const page)
{
    static const struct { int flag; char id; } flags[] = {
        { PAGE_READ, 'r'   },
        { PAGE_WRITE, 'w'  },
        { PAGE_CPU, 'c'    },
        { PAGE_TEXT, 't'   },
        { PAGE_DEVICE, 'd' },
    };

    printf("addr=%p vaddr=0x%04x flags=", page->addr, page->vaddr);

    for (size_t i = 0; i < 5; i++) {
        if (page->flags & flags[i].flag)
            putc(flags[i].id, stdout);
    }

    if (page->write_handler)
        putc('h', stdout);
    putc('\n', stdout);
}

void dbytes(void *addr, size_t amount)
{
    /*
     * To make this look nice, bytes are grouped to 16 per line, simmilar to
     * what xxd or hexdump does. To get the amount of 16 byte groups, we only
     * have to move the amount by 4 bits right. Any bytes left are printed on
     * the last line.
     */
    size_t lines, rest;

    lines = amount >> 4;
    for (size_t i = 0; i < lines; i++) {
        for (size_t j = 0; j < 16; j++) {
            printf("%02hhx", ((char *) addr)[i*16+j]);
            if (j % 2 != 0)
                putc(' ', stdout);
        }
        printf("\n");
    }

    rest = (lines << 4) ^ amount;
    for (size_t i = 0; i < rest; i++) {
        printf("%02hhx", ((char *) addr)[(lines << 4) + i]);
        if (i % 2 != 0)
            putc(' ', stdout);
    }

    if (rest)
        putc('\n', stdout);
}
