/* Irid assembler internal header.
   Copyright (C) 2021 bellrise */

#include <irid/common.h>
#include <irid/asm.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define _Unused     __attribute__((unused))

#ifdef DEBUG
# define info(...)      _info_impl("irid-asm", __func__, __VA_ARGS__)
#else
# define info(...)
#endif

#define die(...)        _die_impl("irid-asm", __VA_ARGS__)
#define warn(...)       _warn_impl("irid-asm", __VA_ARGS__)

struct asm_ctx;
struct hashmap;

/*
 * Runtime information. Only a single global instace is created (GLOB_RUNTIME),
 * storing all data required for the program to work. This is an alternative to
 * global variables, so only one is accessible.
 */
struct runtime
{
    int             nsources;
    char            **sources;
    struct asm_ctx  **contexts;
};

/*
 * Assembler context. Each translation unit gets its own assembler context
 * struct allocated.
 */
struct asm_ctx
{
    FILE    *in;                /* input */
    FILE    *out;               /* output */
    char    *path;              /* path to file */
    int     pos[2];             /* line, pos */
};

/* Hash map element. */
struct hm_elem
{
    char *key;                  /* hashmap key */

    union
    {
        void    *vptr;          /* any pointer */
        char    *vstr;          /* pointer to string */
        size_t  vnum;           /* any number */
    };

    struct hm_elem *next;       /* pointer to next element */
};

/*
 * A hashmap stores a key-value pair in slots. To get the index of a element,
 * the string key is hashed.
 */
struct hashmap
{
    size_t          size;       /* amount of filled elements */
    size_t          space;      /* space in the hashmap */
    struct hm_elem  *slots;     /* array of slots */
};

/* Global runtime instance. */
extern struct runtime *GLOB_RUNTIME;

/*
 * Parse command-line arguments and put them in the runtime struct.
 *
 * @param   rt          assembler runtime info
 * @param   argc        argc
 * @param   argv        argv
 */
int argparse(struct runtime *rt, int argc, char **argv);

/*
 * Hash a null-terminated string and return an arch-bit unsigned integer.
 *
 * @param   str         string to hash
 */
size_t hash(char *str);

/*
 * This subroutine is called after exit() is called. All allocated global
 * resources are free'd using this function. Passed to atexit() in main().
 */
void free_resources();
