/* Emulator internal definitions.
   Copyright (C) 2021 bellrise */
#ifndef EMUL_H
#define EMUL_H

#include <irid/emul.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define _Unused         __attribute__((unused))

/* Runtime information. */
struct runtime
{
    int     argc;
    char  **argv;
    char   *binfile;
    int     is_verbose;
};

int argparse(struct runtime *rt, int argc, char **argv);
int die(const char *fmt, ...);
int warn(const char *fmt, ...);
int _info_internal(const char *func, const char *fmt, ...);

#ifdef DEBUG
/* info() is only called if DEBUG is defined. */
# define info(...)  _info_internal(__func__, __VA_ARGS__)
#else
# define info(...)
#endif


#endif /* EMUL_H */
