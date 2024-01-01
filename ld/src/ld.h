/* ld.h - Irid linker
   Copyright (c) 2023-2024 bellrise */

#pragma once

#include <irid/iof.h>
#include <stdbool.h>
#include <stddef.h>

#define LD_VER_MAJOR 0
#define LD_VER_MINOR 2

#define INVALID_ADDR ((int) -1)

/* strlist */

struct strlist
{
    char **strings;
    size_t size;
};

void strlist_append(struct strlist *, const char *string);
void strlist_free(struct strlist *);

/* buffer */

enum buffer_type
{
    BUFFER_TYPE_MALLOC,
    BUFFER_TYPE_MMAP
};

struct buffer
{
    void *mem;
    size_t size;
    int type;
};

struct buffer *buffer_new();
void buffer_resize(struct buffer *, size_t size);
void buffer_mmap(struct buffer *, const char *path);
void buffer_write_file(struct buffer *, const char *path);
void buffer_free_contents(struct buffer *);
void buffer_free(struct buffer *);

/* options */

struct options
{
    const char *output;
    struct strlist inputs;
    bool dump_symbols;
    bool only_exported;
    bool dump_header;
    bool portable;
    bool verbose;
};

void opt_set_defaults(struct options *opts);
void opt_parse(struct options *opts, int argc, char **argv);

/* section & object */

struct ld_section
{
    struct ld_section *next;
    struct iof_section header;
    int file_offset;
    void *base_ptr;
};

struct ld_section *ld_section_new();
void ld_section_free(struct ld_section *);
int ld_section_symbol_reladdr(struct ld_section *, int strid);
const char *ld_section_name(struct ld_section *);
const char *ld_section_string_by_id(struct ld_section *, int strid);
struct iof_string *ld_section_string_record_by_id(struct ld_section *,
                                                  int strid);

struct ld_object
{
    struct ld_object *next;
    struct ld_section *first_section;
    struct buffer file;
    char *source_path;
    struct iof_header object_header;
};

struct ld_object *ld_object_new(struct ld_object *parent_or_null);
void ld_object_from_file(struct ld_object *, const char *path);
void ld_object_free(struct ld_object *);
struct ld_section *ld_object_section_new(struct ld_object *);
void ld_object_dump_header(struct ld_object *);
void ld_object_dump_symbols(struct ld_object *, bool only_exports);
void ld_object_dump_header_p(struct ld_object *);
void ld_object_dump_symbols_p(struct ld_object *, bool only_exports);

/* linker */

struct ld_section_entry
{
    struct ld_section *section;
    struct ld_object *parent;
    struct ld_region *region;
};

enum ld_region_type
{
    LD_REGION_FREE,
    LD_REGION_ALLOCATED,
};

struct ld_region
{
    struct ld_region *next;
    struct ld_region *prev;
    int type;
    int start;
    int size;
    struct ld_section_entry *section;
};

struct ld_region *ld_region_new();
bool ld_region_contains(struct ld_region *, struct ld_region *other);
int ld_region_insert(struct ld_region *into, struct ld_region *region);
void ld_region_free(struct ld_region *);

struct ld_symbol
{
    char *symbol;
    int real_addr;
    struct ld_section_entry *entry;
};

struct ld_linker
{
    struct ld_object *first_object;
    struct ld_section_entry **entries;
    int n_entries;
    struct ld_symbol **symbols;
    int n_symbols;
    bool verbose;
    struct ld_region *_region_chain;
    struct buffer *output;
};

struct ld_linker *ld_linker_new();
void ld_linker_add_object_chain(struct ld_linker *,
                                struct ld_object *object_chain);
void ld_linker_link(struct ld_linker *, const char *output_path);
void ld_linker_free(struct ld_linker *);

/* debug/die */

#if defined DEBUG
# define debug(...) debug_impl(__VA_ARGS__)
#else
# define debug(...)
#endif

void debug_impl(const char *fmt, ...);
void die(const char *fmt, ...);
