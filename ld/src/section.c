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
