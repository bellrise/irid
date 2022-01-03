/* Hashmap implementation.
   Copyright (C) 2021-2022 bellrise */

#include "asm.h"


static void drop_elem(struct hashmap *hm, struct hm_elem *elem);
struct hm_elem *continue_search(struct hm_elem *parent, int *res, size_t hash_);


void hm_init(struct hashmap *hm)
{
    memset(hm, 0, sizeof(struct hashmap));

    hm->space = HM_INIT_SIZE;
    hm->slots = calloc(sizeof(struct hm_elem *), hm->space);
}

void hm_free(struct hashmap *hm)
{
    /*
     * We can't use the iterator mechanism, because we're removing the
     * elements. Instead, we use the recursive drop_elem subroutine.
     */
    for (size_t i = 0; i < hm->space; i++) {
        if (!hm->slots[i])
            continue;
        drop_elem(hm, hm->slots[i]);
    }
}

struct hm_elem *hm_acquire(struct hashmap *hm, char *key)
{
    struct hm_elem *slot;
    size_t index;
    int res;

    index = hash(key) % hm->space;
    if (hm->slots[index]) {
        slot = continue_search(hm->slots[index], &res, index);
    }
}

struct hm_elem *hm_get(struct hashmap *hm, char *key)
{

}

struct hm_elem *hm_next(struct hashmap *hm)
{

}

int hm_remove(struct hashmap *hm, char *key)
{
    return 1;
}

static void drop_elem(struct hashmap *hm, struct hm_elem *elem)
{
    if (!elem)
        return;

    if (elem->next)
        drop_elem(hm, elem->next);

    if (hm->free_call)
        hm->free_call(elem);

    free(elem);
}

struct hm_elem *continue_search(struct hm_elem *parent, int *res, size_t hash_)
{
    /*
     * Continue searching for an empty slot, starting from `parent`.
     * This function is recursive and returns a pointer to the first
     * empty slot.
     *
     * Sets res to 1 if the item already exists.
     */
    if (hash(parent->key) == hash_) {
        *res = 1;
        return parent;
    }

    if (parent->next)
        return continue_search(parent->next, res, hash_);

    *res = 0;
    return parent->next;
;}
