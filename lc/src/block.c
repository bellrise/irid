/* Leaf compiler
   Copyright (c) 2023 bellrise */

#include "lc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void *block_alloc(struct block *parent, int type)
{
    struct block *block;
    size_t alloc_size;

    switch (type) {
    case BLOCK_FUNC:
        alloc_size = sizeof(struct block_func);
        break;
    default:
        alloc_size = sizeof(struct block);
    }

    block = ac_alloc(global_ac, alloc_size);
    block->type = type;
    block->parent = parent;

    if (parent)
        block_add_child(parent, block);

    return block;
}

void block_add_child(struct block *parent, struct block *child)
{
    struct block *walker;

    if (!parent->child) {
        parent->child = child;
        return;
    }

    walker = parent->child;
    while (walker->next)
        walker = walker->next;

    walker->next = child;
}

void block_add_next(struct block *block, struct block *next)
{
    struct block *walker;

    walker = block;
    while (walker->next)
        walker = walker->next;

    walker->next = next;
}

const char *block_name(struct block *block)
{
    const char *names[] = {"NULL", "FILE_START", "FUNC", "FUNC_PREAMBLE",
                           "FUNC_EPILOGUE"};
    return names[block->type];
}

static void func_info(struct block_func *self)
{
    printf(" %s", self->label);
}

static void block_tree_dump_level(struct block *block, int level)
{
    for (int i = 0; i < level; i++)
        fputs("  ", stdout);

    printf("\033[34m%s\033[0m", block_name(block));

    switch (block->type) {
    case BLOCK_FUNC:
        func_info((struct block_func *) block);
        break;
    };

    fputc('\n', stdout);

    if (block->child)
        block_tree_dump_level(block->child, level + 1);

    if (block->next)
        block_tree_dump_level(block->next, level);
}

void block_tree_dump(struct block *block)
{
    block_tree_dump_level(block, 0);
}
