/* link
   Copyright (c) 2023 bellrise */

#include "ld.h"

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

void ld_linker_link(struct ld_linker *self, const char *output_path) { }
