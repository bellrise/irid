/* Leaf compiler
   Copyright (c) 2023 bellrise */

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
    for (int i = 0; i < level; i++)
        fputs("  ", stdout);

    printf("\033[31m%s\033[0m \033[33m%s\033[0m ", type_name(self->type),
           self->name);
    switch (self->type) {
    case TYPE_INTEGER:
        printf("bit_width=%d\n", ((struct type_integer *) self)->bit_width);
        break;
    case TYPE_POINTER:
        printf("&\n");
        type_dump_level(((struct type_pointer *) self)->base_type, level + 1);
        break;
    default:
        fputc('\n', stdout);
    }
}

void type_dump(struct type *self)
{
    type_dump_level(self, 0);
}

void type_dump_inline(struct type *self)
{
    struct type *base_type;
    int derefs;

    if (self == NULL) {
        printf("\033[31mnull\033[0m");
        return;
    }

    derefs = 0;
    base_type = self;

    while (base_type->type == TYPE_POINTER) {
        base_type = ((struct type_pointer *) base_type)->base_type;
        derefs++;
    }

    printf("\033[31m%s", base_type->name);

    for (int i = 0; i < derefs; i++)
        fputc('&', stdout);

    printf("\033[0m");
}

const char *type_name(int type_kind)
{
    const char *names[] = {"NULL", "INTEGER", "POINTER"};
    return names[type_kind];
}

void type_register_add_builtin(struct type_register *self)
{
    struct type_integer *type;

    type = type_alloc(self, TYPE_INTEGER);
    type->head.name = string_copy("int", 3);
    type->bit_width = 16;

    type = type_alloc(self, TYPE_INTEGER);
    type->head.name = string_copy("char", 4);
    type->bit_width = 8;
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
