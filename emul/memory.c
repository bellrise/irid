/* Code for handling memory.
   Copyright (C) 2021-2022 bellrise */

#include <irid/arch.h>
#include <sys/mman.h>
#include <unistd.h>
#include <malloc.h>
#include "emul.h"


void bank_alloc(struct memory_bank *bank, size_t amount, ir_word vaddr_offt)
{
    if (amount % 4 != 0)
        warn("unaligned page amount (%d empty)", 4 - (amount % 4));

    bank->pages = amount;
    bank->base_ptr = mmap(NULL, amount * IRID_PAGE_SIZE,
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (bank->base_ptr == MAP_FAILED)
        die("failed to map memory bank (%zu pages)", amount);

    bank->page_info = calloc(amount, sizeof(struct page_info));
    if (!bank->page_info)
        die("failed to alloc page_info array");

    /*
     * Set some default flags for each page. This is nice to not have
     * to mark each page as readable/writable.
     */

    for (size_t i = 0; i < amount; i++) {
        bank->page_info[i].flags = PAGE_READ | PAGE_WRITE;
        bank->page_info[i].vaddr = i * IRID_PAGE_SIZE + vaddr_offt;
        bank->page_info[i].addr = i * IRID_PAGE_SIZE + bank->base_ptr;
    }
}

void bank_free(struct memory_bank *bank)
{
    munmap(bank->base_ptr, bank->pages * IRID_MAX_PAGES);
    free(bank->page_info);
}
