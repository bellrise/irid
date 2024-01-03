/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *type_alloc(struct type_register *self, int kind)
{
    size_t alloc_size;
    struct type *type;

    switch (kind) {
    case TYPE_INTEGER:
        alloc_size = sizeof(struct type_integer);
        break;
    case TYPE_POINTER:
        alloc_size = sizeof(struct type_pointer);
        break;
    case TYPE_STRUCT:
        alloc_size = sizeof(struct type_struct);
        break;
    default:
        alloc_size = sizeof(struct type);
        break;
    }

    type = ac_alloc(global_ac, alloc_size);
    type->type = kind;

    self->types = ac_realloc(global_ac, self->types,
                             sizeof(struct type *) * (self->n_types + 1));
    self->types[self->n_types++] = type;

    return type;
}

void type_dump_level(struct type *self, int level)
{
    struct type_struct *struct_type;

    for (int i = 0; i < level; i++)
        fputs("  ", stdout);

    printf("\033[31m%s\033[0m \033[33m%s\033[0m ", type_kind_name(self->type),
           self->name);
    switch (self->type) {
    case TYPE_INTEGER:
        printf("bit_width=%d\n", ((struct type_integer *) self)->bit_width);
        break;
    case TYPE_POINTER:
        printf("&\n");
        type_dump_level(((struct type_pointer *) self)->base_type, level + 1);
        break;
    case TYPE_STRUCT:
        printf("\033[31mtype %s\033[0m\n", self->name);
        struct_type = (struct type_struct *) self;
        for (int i = 0; i < struct_type->n_fields; i++)
            type_dump_level(struct_type->field_types[i], level + 1);
        break;
    default:
        fputc('\n', stdout);
    }
}

void type_dump(struct type *self)
{
    type_dump_level(self, 0);
}

void parsed_type_dump_inline(struct parsed_type *self)
{
    if (self == NULL) {
        printf("\033[31mnull\033[0m");
        return;
    }

    printf("\033[31m%s%s\033[0m", self->name, self->is_pointer ? "&" : "");
}

const char *type_kind_name(int type_kind)
{
    const char *names[] = {"NULL", "INTEGER", "POINTER"};
    return names[type_kind];
}

const char *type_repr(struct type *self)
{
    char *name;

    name = ac_alloc(global_ac, 64);
    snprintf(name, 64, "%s%s", self->name,
             self->type == TYPE_POINTER ? "&" : "");

    return name;
}

bool type_is_null(struct type *self)
{
    if (!self)
        return true;
    return self->type == TYPE_NULL;
}

int type_size(struct type *self)
{
    if (self->type == TYPE_INTEGER)
        return ((struct type_integer *) self)->bit_width >> 3;
    if (self->type == TYPE_POINTER)
        return 2;

    die("cannot calculate type_size for %s", type_kind_name(self->type));
    return 0;
}

struct type *type_register_resolve(struct type_register *self, const char *name)
{
    for (int i = 0; i < self->n_types; i++) {
        if (!strcmp(self->types[i]->name, name))
            return self->types[i];
    }

    return NULL;
}

struct type_pointer *type_register_add_pointer(struct type_register *self,
                                               struct type *base_type)
{
    struct type_pointer *same_pointer_type;
    struct type_pointer *pointer_type;

    /* Check if the pointer type for this base_type already exists. */

    same_pointer_type = NULL;

    for (int i = 0; i < self->n_types; i++) {
        if (self->types[i]->type == TYPE_POINTER) {
            if (((struct type_pointer *) self->types[i])->base_type
                == base_type) {
                same_pointer_type = (struct type_pointer *) self->types[i];
                break;
            }
        }
    }

    if (same_pointer_type)
        return same_pointer_type;

    /* Otherwise, add it to the register. */

    pointer_type = type_alloc(self, TYPE_POINTER);
    pointer_type->base_type = base_type;

    return pointer_type;
}

struct parsed_type *parsed_type_register_new(struct parsed_type_register *self,
                                             const char *name, bool is_pointer)
{
    struct parsed_type *type;

    self->types =
        ac_realloc(global_ac, self->types,
                   sizeof(struct parsed_type *) * (self->n_types + 1));

    type = ac_alloc(global_ac, sizeof(struct parsed_type));
    type->name = string_copy(name, strlen(name));
    type->is_pointer = is_pointer;

    self->types[self->n_types++] = type;

    return type;
}
