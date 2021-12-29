/* Emulator internal definitions.
   Copyright (C) 2021 bellrise */

#ifndef EMUL_H
#define EMUL_H

#include "emul_graphics.h"
#include <irid/common.h>
#include <irid/emul.h>
#include <irid/arch.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>

#define _Unused         __attribute__((unused))

#ifdef DEBUG
/* info() is only called if DEBUG is defined. */
# define info(...)      _info_impl("irid-emul", __func__, __VA_ARGS__)
#else
# define info(...)
#endif

#define die(...)        _die_impl("irid-emul", __VA_ARGS__)
#define warn(...)       _warn_impl("irid-emul", __VA_ARGS__)


struct runtime;
struct page_info;
struct memory_bank;
struct emulator_ctx;

/* Runtime information. */

struct runtime
{
    int     argc;
    char    **argv;
    char    *binfile;
    bool    is_headless;
    struct eg_window *win;
};

/* Page flags. */

#define PAGE_NULL           0x0000
#define PAGE_WRITE          0x0001
#define PAGE_READ           0x0002
#define PAGE_CPU            0x0004
#define PAGE_TEXT           0x0008
#define PAGE_DEVICE         0x0010

/*
 * Each mapped 1024B page has a page_info struct assigned to it. The
 * write_handler is a function which is called on every write to a page that
 * has one assigned. It may return an error code
 */
struct page_info
{
    int      flags;
    ir_word  vaddr;
    char    *addr;
    int    (*write_handler) (struct page_info *page, ir_word vaddr);
};

/*
 * A memory bank contains n 1024B pages, each contaning a seperate page_info
 * struct with some flags and callbacks assigned. The base_ptr is the real
 * pointer into memory the emulator works in.
 */
struct memory_bank
{
    char             *base_ptr;
    size_t            pages;
    struct page_info *page_info;
};

/*
 * Emulator context structure. Contains pointers to the memory bank and some
 * other emulator settings.
 */
struct emulator_ctx
{
    struct memory_bank  *mem;
    struct eg_window    *win;
    struct irid_reg     *regs;
    jmp_buf             crash_ret;
    bool                is_silent;
};

/*
 * Allocate n Irid pages and store them in the memory bank array. The vaddr_offt
 * is the amount of bytes each virtual address with be offset from the base_ptr
 * of the memory bank.
 *
 * @param   bank        memory bank
 * @param   amount      amount of pages to allocate
 * @param   vaddr_offt  virutal address offset from page 0
 */
void bank_alloc(struct memory_bank *bank, size_t amount, ir_word vaddr_offt);

/*
 * Free the whole bank.
 *
 * @param   bank        memory bank to free
 */
void bank_free(struct memory_bank *bank);

/*
 * Read either 8 or 16 bytes from the memory bank, in the vaddr offset.
 *
 * @param   mem         memory bank to read from
 * @param   vaddr       virtual address of the memory
 */
ir_half read8(struct memory_bank *mem, ir_word vaddr);
ir_word read16(struct memory_bank *mem, ir_word vaddr);

/*
 * Write either 8 or 16 bytes to the memory bank, to the vaddr offset
 * Additionally, if a write_handler is defined, it is called before the
 * write. If the write is not permitted, the emulator will die.
 *
 * @param   mem         memory bank to write to
 * @param   vaddr       virtual address of the memory
 * @param   val         value to write
 */
void write8(struct memory_bank *mem, ir_word vaddr, ir_half val);
void write16(struct memory_bank *mem, ir_word vaddr, ir_word val);

/*
 * Parse command-line arguments and put them in the runtime struct.
 *
 * @param   rt          pointer to a runtime struct
 * @param   argc        argc
 * @param   argv        argv
 */
int argparse(struct runtime *rt, int argc, char **argv);

/*
 * Print the physical & virutal address and page flags. There are 6 flags:
 * r - readable, w - writable, c - cpu page, t - text page, d - device mapped,
 * h - has write handler.
 *
 * @param   page        pointer to page_info
 */
void page_info_d(const struct page_info *const page);


#endif /* EMUL_H */
