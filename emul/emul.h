/* Emulator internal definitions.
   Copyright (C) 2021 bellrise */

#ifndef EMUL_H
#define EMUL_H

#include <irid/emul.h>
#include <irid/arch.h>
#include <stdlib.h>
#include <stdio.h>

#define _Unused         __attribute__((unused))

#ifdef DEBUG
/* info() is only called if DEBUG is defined. */
# define info(...)  _info_internal(__func__, __VA_ARGS__)
#else
# define info(...)
#endif

/* Runtime information. */
struct runtime
{
    int     argc;
    char  **argv;
    char   *binfile;
    int     is_verbose;
};

/* Page flags. */
#define PAGE_NULL           0x0000
#define PAGE_WRITE          0x0001
#define PAGE_READ           0x0002
#define PAGE_CPU            0x0004
#define PAGE_TEXT           0x0008
#define PAGE_DEVICE         0x0010

/* Each mapped 1024B page has a page_info struct assigned to it. */
struct page_info
{
    int      flags;
    ir_word  vaddr;
    char    *addr;
    int    (*write_handler) (struct page_info *self, ir_word vaddr);
};

/* A memory bank contains n 1024B pages, each contaning a seperate page_info
   struct with some flags and callbacks assigned. */
struct memory_bank
{
    char             *base_ptr;
    size_t            pages;
    struct page_info *page_info;
};

/* Allocate n Irid pages and store them in the memory bank array. The vaddr_offt
   is the amount of bytes each virtual address with be offset from the base_ptr
   of the memory bank. */
void bank_alloc(struct memory_bank *bank, size_t amount, ir_word vaddr_offt);

/* Free the whole bank. */
void bank_free(struct memory_bank *bank);

/* Parse command-line arguments and put them in the runtime struct. */
int argparse(struct runtime *rt, int argc, char **argv);

/* Print an error message and exit. */
int die(const char *fmt, ...);

/* Print a warning. */
int warn(const char *fmt, ...);

/* Print an informational message. Be sure to call the info() macro instead,
   which automatically disables this function is DEBUG is not defined. */
int _info_internal(const char *func, const char *fmt, ...);

/* Print the physical & virutal address and page flags. There are 6 flags:
   r - readable, w - writable, c - cpu page, t - text page, d - device mapped,
   h - has write handler. */
void page_info_d(const struct page_info *const page);

/* Print n bytes in hex format, 16 bytes per line. */
void dbytes(void *addr, size_t amount);


#endif /* EMUL_H */
