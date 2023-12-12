/* region
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <stdlib.h>

struct ld_region *ld_region_new()
{
    struct ld_region *res;

    res = calloc(1, sizeof(*res));
    res->type = LD_REGION_FREE;

    return res;
}

bool ld_region_contains(struct ld_region *self, struct ld_region *other)
{
    if (self->size < other->size)
        return false;

    if (other->start < self->start)
        return false;

    if (self->size - (other->start - self->start) < other->size)
        return false;

    return true;
}

void insert_before(struct ld_region *self, struct ld_region *prev)
{
    prev->next = self;

    if (self->prev)
        self->prev->next = prev;
    self->prev = prev;
}

void insert_after(struct ld_region *self, struct ld_region *next)
{
    next->prev = self;

    if (self->next)
        self->next->prev = next;
    self->next = next;
}

int ld_region_insert(struct ld_region *into, struct ld_region *region)
{
    struct ld_region *after_region;
    int space_before;
    int space_after;

    if (!ld_region_contains(into, region))
        return 1;

    if (into->type != LD_REGION_FREE)
        return 1;

    space_before = region->start - into->start;
    space_after = into->size - (space_before + region->size);

    if (space_before) {
        /* Place the region after this empty one and shrink it. */
        insert_after(into, region);
        into->size -= region->size;
    }

    else if (space_after) {
        /* Place the region before this empty one and shrink it. */
        insert_before(into, region);
        into->size -= region->size;
        into->start += region->size;
    }

    else if (space_before && space_after) {
        /* Place the region in the middle of this empty one. */
        insert_after(into, region);
        into->size -= region->size - space_after;

        after_region = ld_region_new();
        after_region->size = space_after;
        after_region->start = region->start + region->size;
        insert_after(region, after_region);
    }

    else {
        /* Replace the current region with the new one. */
        if (into->prev)
            into->prev->next = region;
        if (into->next)
            into->next->prev = region;

        into->next = NULL;
        into->prev = NULL;

        ld_region_free(into);
    }

    return 0;
}

void ld_region_free(struct ld_region *self)
{
    if (!self)
        return;

    /* Remove ourselfs from the chain */
    if (self->next)
        self->next->prev = self->prev;
    if (self->prev)
        self->prev->next = self->next;

    free(self);
}
