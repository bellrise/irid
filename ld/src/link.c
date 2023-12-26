/* link
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <irid/arch.h>
#include <stdlib.h>
#include <string.h>

struct ld_linker *ld_linker_new()
{
    return calloc(1, sizeof(struct ld_linker));
}

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

static void append_section_entry(struct ld_linker *self,
                                 struct ld_object *parent,
                                 struct ld_section *section)
{
    struct ld_section_entry *entry;

    entry = calloc(1, sizeof(*entry));
    entry->parent = parent;
    entry->section = section;

    self->entries = realloc(self->entries, sizeof(struct ld_section_entry *)
                                               * (self->n_entries + 1));
    self->entries[self->n_entries++] = entry;
}

static void collect_sections(struct ld_linker *self)
{
    struct ld_object *owalker;
    struct ld_section *swalker;

    owalker = self->first_object;
    while (owalker) {
        swalker = owalker->first_section;
        while (swalker) {
            append_section_entry(self, owalker, swalker);
            swalker = swalker->next;
        }

        owalker = owalker->next;
    }
}

static void create_empty_region(struct ld_linker *self)
{
    self->_region_chain = ld_region_new();
    self->_region_chain->start = 0;
    self->_region_chain->size = IRID_MAX_ADDR;
}

static struct ld_region *get_first_region(struct ld_linker *self)
{
    while (self->_region_chain->prev)
        self->_region_chain = self->_region_chain->prev;
    return self->_region_chain;
}

static struct ld_region *find_region_with_addr(struct ld_linker *self, int addr)
{
    struct ld_region *region;

    region = get_first_region(self);
    while (region) {
        if (addr >= region->start && addr < (region->start + region->size))
            return region;
        region = region->next;
    }

    return NULL;
}

static void dump_regions(struct ld_linker *self)
{
    struct ld_region *walker;

    walker = get_first_region(self);
    while (walker) {
        debug("[0x%04x:0x%04x] %s", walker->start, walker->start + walker->size,
              walker->type == LD_REGION_FREE ? "FREE" : "ALLOC");
        walker = walker->next;
    }
}

static void place_static_regions(struct ld_linker *self)
{
    struct ld_section_entry *entry;
    struct ld_region *new_region;
    struct ld_section *section;
    struct ld_region *region;

    for (int i = 0; i < self->n_entries; i++) {
        section = self->entries[i]->section;
        entry = self->entries[i];

        /* Only place static regions */
        if (!(section->header.s_flag & IOF_SFLAG_STATIC_ORIGIN))
            continue;

        region = find_region_with_addr(self, section->header.s_origin);
        if (!region) {
            die("cannot find region with address 0x%04x",
                section->header.s_origin);
        }

        if (region->type != LD_REGION_FREE) {
            die("%s:%s[0x%04x:0x%04x] overlaps %s:%s[0x%04x:0x%04x]",
                entry->parent->source_path, ld_section_name(section),
                section->header.s_origin, section->header.s_code_size,
                region->section->parent->source_path,
                ld_section_name(region->section->section), region->start,
                region->size);
        }

        new_region = ld_region_new();
        new_region->type = LD_REGION_ALLOCATED;
        new_region->size = section->header.s_code_size;
        new_region->start = section->header.s_origin;
        new_region->section = entry;
        entry->region = new_region;

        if (ld_region_insert(region, new_region))
            die("failed to insert region");
    }
}

static struct ld_section_entry *
largest_nonplaced_section(struct ld_linker *self)
{
    struct ld_section_entry *entry = NULL;
    int size = 0;

    for (int i = 0; i < self->n_entries; i++) {
        if (self->entries[i]->region)
            continue;

        if (size < self->entries[i]->section->header.s_code_size) {
            entry = self->entries[i];
            size = self->entries[i]->section->header.s_code_size;
        }
    }

    return entry;
}

static void place_movable_regions(struct ld_linker *self)
{
    struct ld_section_entry *entry;
    struct ld_region *rwalker;
    struct ld_region *new_region;

    /* Place all sections in empty spaces, starting from the largest. */

    while ((entry = largest_nonplaced_section(self))) {
        rwalker = get_first_region(self);
        while (rwalker) {
            if (rwalker->type == LD_REGION_FREE
                && rwalker->size >= entry->section->header.s_code_size) {
                break;
            }

            rwalker = rwalker->next;
        }

        if (!rwalker) {
            die("failed to find enough space for section %s:%s",
                entry->parent->source_path, ld_section_name(entry->section));
        }

        new_region = ld_region_new();
        new_region->type = LD_REGION_ALLOCATED;
        new_region->size = entry->section->header.s_code_size;
        new_region->start = rwalker->start;
        new_region->section = entry;
        entry->region = new_region;

        if (ld_region_insert(rwalker, new_region))
            die("failed to insert region");
    }
}

static void add_symbol(struct ld_linker *self, struct ld_symbol *sym)
{
    self->symbols = realloc(self->symbols,
                            sizeof(struct ld_symbol *) * (self->n_symbols + 1));
    self->symbols[self->n_symbols++] = sym;
}

static void collect_symbols_from_section(struct ld_linker *self,
                                         struct ld_section_entry *entry)
{
    struct iof_export *exportv;
    struct ld_symbol *symbol;

    exportv = entry->section->base_ptr + entry->section->header.s_exports_addr;

    for (int i = 0; i < entry->section->header.s_exports_count; i++) {
        symbol = calloc(1, sizeof(*symbol));
        symbol->symbol =
            strdup(ld_section_string_by_id(entry->section, exportv[i].e_strid));
        symbol->entry = entry;
        symbol->real_addr = entry->region->start + exportv[i].e_offset;
        add_symbol(self, symbol);
    }
}

static void dump_symbols(struct ld_linker *self)
{
    for (int i = 0; i < self->n_symbols; i++) {
        debug("0x%04x %s", self->symbols[i]->real_addr,
              self->symbols[i]->symbol);
    }
}

static struct ld_symbol *resolve_global_symbol(struct ld_linker *self,
                                               const char *name)
{
    for (int i = 0; i < self->n_symbols; i++) {
        if (!strcmp(self->symbols[i]->symbol, name))
            return self->symbols[i];
    }

    return NULL;
}

static struct ld_symbol *resolve_local_symbol(struct ld_section_entry *entry,
                                              const char *name)
{
    struct ld_symbol *symbol;
    struct iof_symbol *symv;
    const char *cmpname;

    symv = entry->section->base_ptr + entry->section->header.s_symbols_addr;
    cmpname = NULL;

    for (int i = 0; i < entry->section->header.s_symbols_count; i++) {
        cmpname = ld_section_string_by_id(entry->section, symv[i].l_strid);
        if (!strcmp(cmpname, name)) {
            symbol = calloc(1, sizeof(*symbol));
            symbol->symbol = strdup(name);
            symbol->entry = entry;
            symbol->real_addr = entry->region->start + symv[i].l_addr;
            return symbol;
        }
    }

    return NULL;
}

static void link_section(struct ld_linker *self, struct ld_section_entry *entry)
{
    /* Linking a section is pretty simple - find all link points and add the
       correct pointers to local functions or exported functions. */

    struct iof_link *linkv;
    struct ld_symbol *symbol;
    const char *symname;
    bool was_local_symbol;
    u16 *pointer;

    linkv = entry->section->base_ptr + entry->section->header.s_links_addr;

    for (int i = 0; i < entry->section->header.s_links_count; i++) {
        symname = ld_section_string_by_id(entry->section, linkv[i].l_strid);

        symbol = resolve_local_symbol(entry, symname);
        was_local_symbol = true;

        if (!symbol) {
            symbol = resolve_global_symbol(self, symname);
            was_local_symbol = false;
        }

        if (!symbol)
            die("`%s` not found", symname);

        pointer = self->output->mem + entry->region->start + linkv[i].l_addr;
        *pointer = symbol->real_addr;

        if (was_local_symbol) {
            free(symbol->symbol);
            free(symbol);
        }
    }
}

static void create_output_buffer(struct ld_linker *self)
{
    struct ld_region *walker;
    int maxaddr;

    walker = get_first_region(self);
    maxaddr = 0;

    while (walker) {
        if (walker->type == LD_REGION_ALLOCATED)
            maxaddr = walker->start + walker->size;
        walker = walker->next;
    }

    self->output = buffer_new();
    buffer_resize(self->output, maxaddr);
}

static void copy_code_sections(struct ld_linker *self)
{
    struct ld_region *region;
    void *output_buffer;
    void *code_segment;

    region = get_first_region(self);
    while (region) {
        if (region->type == LD_REGION_ALLOCATED) {
            /* Copy code section into the selected region. */
            output_buffer = self->output->mem + region->start;
            code_segment = region->section->section->base_ptr
                         + region->section->section->header.s_code_addr;
            memcpy(output_buffer, code_segment, region->size);
        }

        region = region->next;
    }
}

static void zero_free_sections(struct ld_linker *self)
{
    struct ld_region *region;
    void *output_buffer;

    /* Write all but the last free section. */

    region = get_first_region(self);
    while (region->next) {
        if (region->type == LD_REGION_FREE) {
            output_buffer = self->output->mem + region->start;
            memset(output_buffer, 0, region->size);
        }

        region = region->next;
    }
}

void ld_linker_link(struct ld_linker *self, const char *output_path)
{
    collect_sections(self);
    create_empty_region(self);

    /* Map sections into memory regions. */
    place_static_regions(self);
    place_movable_regions(self);

    for (int i = 0; i < self->n_entries; i++)
        collect_symbols_from_section(self, self->entries[i]);

    if (self->verbose) {
        dump_regions(self);
        dump_symbols(self);
    }

    create_output_buffer(self);
    copy_code_sections(self);
    zero_free_sections(self);

    for (int i = 0; i < self->n_entries; i++)
        link_section(self, self->entries[i]);

    buffer_write_file(self->output, output_path);
}

void ld_linker_free(struct ld_linker *self)
{
    struct ld_region *walker;
    struct ld_region *next;

    if (!self)
        return;

    /* Go to the last region */
    walker = get_first_region(self);
    while (walker) {
        next = walker->next;
        ld_region_free(walker);
        walker = next;
    }

    for (int i = 0; i < self->n_entries; i++)
        free(self->entries[i]);

    for (int i = 0; i < self->n_symbols; i++) {
        free(self->symbols[i]->symbol);
        free(self->symbols[i]);
    }

    buffer_free(self->output);
    free(self->entries);
    free(self->symbols);
    free(self);
}
