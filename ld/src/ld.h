/* ld.h - Irid linker
   Copyright (c) 2023 bellrise */

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
char *ld_section_string_by_id(struct ld_section *, int strid);
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
void ld_object_dump(struct ld_object *);

/* linker */

struct ld_linker
{
    struct ld_object *first_object;
};

void ld_linker_add_object_chain(struct ld_linker *,
                                struct ld_object *object_chain);
void ld_linker_link(struct ld_linker *, const char *output_path);

/* debug/die */

#if defined DEBUG
void debug(const char *fmt, ...);
#else
# define debug(...)
#endif

void die(const char *fmt, ...);
