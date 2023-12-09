/* object
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <stdlib.h>
#include <string.h>

struct ld_object *ld_object_new(struct ld_object *parent_or_null)
{
    struct ld_object *self;

    self = calloc(1, sizeof(*self));
    while (parent_or_null) {
        if (parent_or_null->next == NULL) {
            parent_or_null->next = self;
            break;
        }

        parent_or_null = parent_or_null->next;
    }

    return self;
}

static void load_and_validate_header(struct ld_object *self)
{
    memcpy(&self->object_header, self->file.mem, sizeof(self->object_header));

    if (IOF_FORMAT != 2)
        die("linker implements other format than in iof.h header");
    if (memcmp(self->object_header.h_magic, IOF_MAGIC, 4))
        die("invalid magic bytes in %s", self->source_path);
    if (self->object_header.h_format != IOF_FORMAT) {
        die("unsupported object format %d, linker implements %d",
            self->object_header.h_format, IOF_FORMAT);
    }
}

static void load_section(struct ld_object *self, int ptr)
{
    struct ld_section *section;

    section = ld_object_section_new(self);
    section->base_ptr = self->file.mem + ptr;
    memcpy(&section->header, section->base_ptr, sizeof(section->header));
}

void ld_object_from_file(struct ld_object *self, const char *path)
{
    struct iof_pointer *pointers;

    buffer_mmap(&self->file, path);
    if (!self->file.size)
        die("failed to open file: '%s'", path);

    self->source_path = strdup(path);
    load_and_validate_header(self);

    pointers = self->file.mem + self->object_header.h_section_addr;
    for (size_t i = 0; i < self->object_header.h_section_count; i++)
        load_section(self, pointers[i].p_addr);
}

void ld_object_free(struct ld_object *self)
{
    if (self == NULL)
        return;

    if (self->next)
        ld_object_free(self->next);

    ld_section_free(self->first_section);
    buffer_free_contents(&self->file);

    free(self->source_path);
    free(self);
}

struct ld_section *ld_object_section_new(struct ld_object *self)
{
    struct ld_section *section;
    struct ld_section *walker;

    section = ld_section_new();
    if (!self->first_section) {
        self->first_section = section;
        return section;
    }

    walker = self->first_section;
    while (walker->next)
        walker = walker->next;

    walker->next = section;
    return section;
}
