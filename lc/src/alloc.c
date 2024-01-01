/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AC_LIST_STEP_SIZE 512

void allocator_init(struct allocator *self)
{
    self->allocs = NULL;
    self->size = 0;
    self->space = 0;
}

static void allocator_require_size(struct allocator *self, size_t size)
{
    if (self->space >= size)
        return;

    self->space += AC_LIST_STEP_SIZE;
    self->allocs =
        realloc(self->allocs, sizeof(struct ac_allocation *) * self->space);
}

void allocator_free_all(struct allocator *self)
{
    for (size_t i = 0; i < self->size; i++)
        free(self->allocs[i]);
    free(self->allocs);

    self->allocs = NULL;
    self->size = 0;
    self->space = 0;
}

void *ac_alloc(struct allocator *self, size_t bytes)
{
    struct ac_allocation *alloc;

    allocator_require_size(self, self->size + 1);

    /* Allocate n more bytes before the actual memory area the user requested,
       where we can store our metadata about the allocation. */

    alloc = malloc(sizeof(*alloc) + bytes);
    alloc->size = bytes;
    alloc->index = self->size;
    self->allocs[self->size++] = alloc;

    memset(alloc->area, 0, alloc->size);

    return alloc->area;
}

void *ac_realloc(struct allocator *self, void *addr, size_t bytes)
{
    struct ac_allocation *new_alloc;
    struct ac_allocation *alloc;

    if (addr == NULL)
        return ac_alloc(self, bytes);

    /* If we want to resize a previous allocation, we have to check if we have
       enough space in the allocation already. */

    alloc =
        (struct ac_allocation *) ((size_t) addr - sizeof(struct ac_allocation));
    if (alloc->size >= bytes)
        return alloc->area;

    /* Otherwise, allocate a fresh buffer, overwriting the previous one. */

    new_alloc = malloc(sizeof(*alloc) + bytes);
    new_alloc->size = bytes;
    new_alloc->index = alloc->index;
    self->allocs[new_alloc->index] = new_alloc;

    memcpy(new_alloc->area, alloc->area, alloc->size);

    /* We can now free the old, unused buffer. */

    free(alloc);

    return new_alloc->area;
}
