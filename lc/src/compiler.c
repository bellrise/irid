/* Leaf compiler
   Copyright (c) 2023 bellrise */

#include "lc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void compile_func(struct compiler *self, struct node_func_def *func)
{
    struct block_func *func_block;

    func_block = block_alloc(self->file_block, BLOCK_FUNC);
    func_block->label = string_copy(func->decl->name, strlen(func->decl->name));
    func_block->exported = true;

    block_alloc((struct block *) func_block, BLOCK_FUNC_PREAMBLE);
    block_alloc((struct block *) func_block, BLOCK_FUNC_EPILOGUE);
}

void compiler_compile(struct compiler *self)
{
    struct node *func_walker;

    self->file_block = block_alloc(NULL, BLOCK_FILE);

    func_walker = self->tree->child;
    while (func_walker) {
        if (func_walker->type == NODE_FUNC_DEF)
            compile_func(self, (struct node_func_def *) func_walker);

        func_walker = func_walker->next;
    }

    block_tree_dump(self->file_block);
}
