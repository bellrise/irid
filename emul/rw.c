/* Memory bank read/write subroutines.
   Copyright (C) 2021-2022 bellrise */

#include "emul.h"


static struct page_info *vaddr_page(struct memory_bank *mem, ir_word vaddr);


ir_half read8(struct memory_bank *mem, ir_word vaddr)
{
    struct page_info *page;
    ir_half res;

    page = vaddr_page(mem, vaddr);
    vaddr -= page->vaddr;
    res = page->addr[vaddr];

    info("0x%02hx 0x%04hx", res, vaddr);

    return res;
}

ir_word read16(struct memory_bank *mem, ir_word vaddr)
{
    struct page_info *page;
    ir_word res;

    page = vaddr_page(mem, vaddr);
    vaddr -= page->vaddr;
    res = * (ir_word *) ((size_t) page->addr + vaddr);

    info("0x%04hx @ 0x%04hx", res, vaddr);

    return res;
}

void write8(struct memory_bank *mem, ir_word vaddr, ir_half val)
{
    struct page_info *page;
    ir_half *addr;

    page = vaddr_page(mem, vaddr);

    if (!(page->flags & PAGE_WRITE))
        die("forbidden write 8 to 0x%04x", vaddr);

    /* If any write handler is defined, be sure to call it. */
    if (page->write_handler)
        page->write_handler(page, vaddr);

    addr = (ir_half *) ((size_t) page->addr + (vaddr - page->vaddr));
    *addr = val;
}

void write16(struct memory_bank *mem, ir_word vaddr, ir_word val)
{
    struct page_info *page;
    ir_word *addr;

    page = vaddr_page(mem, vaddr);
    if (!(page->flags & PAGE_WRITE))
        die("forbidden write 16 to 0x%04x", vaddr);

    /* If any write handler is defined, be sure to call it. */
    if (page->write_handler)
        page->write_handler(page, vaddr);

    addr = (ir_word *) ((size_t) page->addr + (vaddr - page->vaddr));
    *addr = val;
}

static struct page_info *vaddr_page(struct memory_bank *mem, ir_word vaddr)
{
    ir_word paddr;

    paddr = vaddr >> IRID_PAGE_SIZE_BITS;

    if (paddr >= mem->pages)
        die("virtual address 0x%x out of range", vaddr);

    return &mem->page_info[paddr];
}
