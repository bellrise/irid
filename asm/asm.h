/* Irid assembler internal header.
   Copyright (C) 2021 bellrise */

#include <irid/asm.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define _Unused     __attribute__((unused))

#ifdef DEBUG
# define info(...)      _info_internal(__func__, __VA_ARGS__)
#else
# define info(...)
#endif

/* Runtime information. */

struct runtime
{
    int     nsources;
    char    **sources;
};

/*
 * Parse command-line arguments and put them in the runtime struct.
 *
 * @param   rt          assembler runtime info
 * @param   argc        argc
 * @param   argv        argv
 */
int argparse(struct runtime *rt, int argc, char **argv);

int die(const char *fmt, ...);
int warn(const char *fmt, ...);
int _info_internal(const char *func, const char *fmt, ...);
