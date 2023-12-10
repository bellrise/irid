/* section
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <stdlib.h>

struct ld_section *ld_section_new()
{
    return calloc(1, sizeof(struct ld_section));
}

void ld_section_free(struct ld_section *self)
{
    if (self == NULL)
        return;

    if (self->next)
        ld_section_free(self->next);
    free(self);
}

int ld_section_symbol_reladdr(struct ld_section *self, int strid)
{
    struct iof_symbol *symv;

    symv = self->base_ptr + self->header.s_symbols_addr;
    for (int i = 0; i < self->header.s_symbols_count; i++) {
        if (symv[i].l_strid == strid)
            return symv[i].l_addr;
    }

    return INVALID_ADDR;
}

char *ld_section_string_by_id(struct ld_section *self, int strid)
{
    struct iof_string *str;

    str = ld_section_string_record_by_id(self, strid);
    if (!str)
        die("failed to find string with strid=%d\n", strid);

    return (char *) self->base_ptr + str->s_addr;
}

struct iof_string *ld_section_string_record_by_id(struct ld_section *self,
                                                  int strid)
{
    struct iof_string *strv;

    strv = self->base_ptr + self->header.s_strings_addr;
    for (int i = 0; i < self->header.s_strings_count; i++) {
        if (strv[i].s_id == strid)
            return &strv[i];
    }

    return NULL;
}
