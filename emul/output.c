/* Output routines.
   Copyright (C) 2021 bellrise */

#include "emul.h"

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
