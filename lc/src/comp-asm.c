/* Leaf compiler
   Copyright (c) 2024 bellrise */

#include "compiler.h"

void compile_asm_call(struct compiler *self, struct block_func *func,
                      struct node *call)
{
    struct block_asm *asm_block;
    struct node_literal *source_node;

    source_node = (struct node_literal *) call->child->next;
    if (!source_node)
        ce(self, call->place, "missing the source argument");

    if (source_node->head.type != NODE_LITERAL) {
        ce(self, source_node->head.place,
           "__asm takes only literal strings as arguments");
    }

    if (source_node->type != LITERAL_STRING) {
        ce(self, source_node->head.place,
           "__asm takes only literal strings as arguments");
    }

    asm_block = block_alloc((struct block *) func, BLOCK_ASM);
    asm_block->source = string_copy_z(source_node->string_value);
}
