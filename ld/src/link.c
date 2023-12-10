/* link
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <stdlib.h>
#include <string.h>

void ld_linker_add_object_chain(struct ld_linker *self,
                                struct ld_object *object_chain)
{
    struct ld_object *walker;

    if (!self->first_object) {
        self->first_object = object_chain;
        return;
    }

    walker = self->first_object;
    while (walker->next)
        walker = walker->next;
    walker->next = object_chain;
}

static struct buffer *link_single_section(struct ld_section *self)
{
    struct iof_link *linkv;
    struct buffer *buf;
    u16 *addr_pointer;
    int addr;

    buf = buffer_new();
    buffer_resize(buf, self->header.s_code_size);

    memcpy(buf->mem, self->base_ptr + self->header.s_code_addr,
           self->header.s_code_size);

    /* Walk through the link points, and replace the pointers. */

    linkv = self->base_ptr + self->header.s_links_addr;
    for (int i = 0; i < self->header.s_links_count; i++) {
        addr = ld_section_symbol_reladdr(self, linkv[i].l_strid);
        if (addr == INVALID_ADDR) {
            die("symbol '%s' not found in section",
                ld_section_string_by_id(self, linkv[i].l_strid));
        }

        addr_pointer = buf->mem + linkv[i].l_addr;
        *addr_pointer = addr;
    }

    return buf;
}

void ld_linker_link(struct ld_linker *self, const char *output_path)
{
    struct ld_object *object;
    struct buffer *section_buffer;

    object = self->first_object;
    if (object->next)
        die("only a single supported currently");

    section_buffer = link_single_section(object->first_section);
    buffer_write_file(section_buffer, output_path);
    buffer_free(section_buffer);
}
