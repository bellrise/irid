/* Leaf compiler
   Copyright (c) 2024 bellrise */

#include "compiler.h"

static void set_int_value(struct value *val, int integer)
{
    val->value_type = VALUE_IMMEDIATE;
    val->imm_value.width = 16;
    val->imm_value.value = integer;
}

static char *loop_start(struct compiler *self, struct block_func *func_block,
                        struct local *index_local, struct node *start_value)
{
    struct block_store *store_first;
    struct block_label *loop_label;

    store_first = block_alloc((struct block *) func_block, BLOCK_STORE);
    store_first->local = index_local;
    convert_node_into_value(self, func_block, &store_first->value, start_value);

    loop_label = block_alloc((struct block *) func_block, BLOCK_LABEL);
    loop_label->label = ac_alloc(global_ac, 8);
    snprintf(loop_label->label, 8, "L%d", func_block->label_index++);

    return loop_label->label;
}

static void loop_end(struct compiler *self, struct block_func *func_block,
                     struct local *index_local, char *loop_label,
                     struct node *end_value)
{
    struct block_store *inc_index;
    struct block_cmp *cmp_index_block;
    struct block_label *end_loop;
    struct block_jmp *jmp_block;
    struct block_jmp *jmp_loop;
    struct value *inc_left;
    struct value *inc_right;
    char *end_label;

    inc_index = block_alloc((struct block *) func_block, BLOCK_STORE);
    inc_index->local = index_local;
    inc_index->value.value_type = VALUE_OP;

    inc_left = ac_alloc(global_ac, sizeof(struct value));
    inc_right = ac_alloc(global_ac, sizeof(struct value));
    inc_left->value_type = VALUE_LOCAL;
    inc_left->local_value = index_local;

    set_int_value(inc_right, 1);

    inc_index->value.op_value.type = OP_ADD;
    inc_index->value.op_value.left = inc_left;
    inc_index->value.op_value.right = inc_right;

    cmp_index_block = block_alloc((struct block *) func_block, BLOCK_CMP);
    cmp_index_block->left.value_type = VALUE_LOCAL;
    cmp_index_block->left.local_value = index_local;
    convert_node_into_value(self, func_block, &cmp_index_block->right,
                            end_value);

    end_label = ac_alloc(global_ac, 8);
    snprintf(end_label, 8, "E%d", func_block->label_index++);

    jmp_block = block_alloc((struct block *) func_block, BLOCK_JMP);
    jmp_block->type = JMP_EQ;
    jmp_block->dest = end_label;

    jmp_loop = block_alloc((struct block *) func_block, BLOCK_JMP);
    jmp_loop->type = JMP_ALWAYS;
    jmp_loop->dest = loop_label;

    end_loop = block_alloc((struct block *) func_block, BLOCK_LABEL);
    end_loop->label = end_label;
}

void compile_loop(struct compiler *self, struct block_func *func_block,
                  struct node_loop *loop_node)
{
    struct node *start_value;
    struct node *end_value;
    struct node *walker;
    struct local *loop_index;
    char *loop_label;

    loop_index = alloc_local(func_block);
    loop_index->type = type_register_resolve(self->types, "int");
    loop_index->name = string_copy_z(loop_node->index_name);
    loop_index->decl = NULL;

    start_value = loop_node->head.child;
    end_value = start_value->next;

    loop_label = loop_start(self, func_block, loop_index, start_value);

    walker = end_value->next;
    while (walker) {
        place_node(self, func_block, walker);
        walker = walker->next;
    }

    loop_end(self, func_block, loop_index, loop_label, end_value);
}
