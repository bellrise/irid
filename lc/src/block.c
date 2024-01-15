/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

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
    case BLOCK_LOCAL:
        alloc_size = sizeof(struct block_local);
        break;
    case BLOCK_GLOBAL:
        alloc_size = sizeof(struct block_global);
        break;
    case BLOCK_STRING:
        alloc_size = sizeof(struct block_string);
        break;
    case BLOCK_STORE_RETURN:
    case BLOCK_STORE_RESULT:
    case BLOCK_STORE:
        alloc_size = sizeof(struct block_store);
        break;
    case BLOCK_STORE_ARG:
        alloc_size = sizeof(struct block_store_arg);
        break;
    case BLOCK_CALL:
        alloc_size = sizeof(struct block_call);
        break;
    case BLOCK_ASM:
        alloc_size = sizeof(struct block_asm);
        break;
    case BLOCK_LABEL:
        alloc_size = sizeof(struct block_label);
        break;
    case BLOCK_JMP:
        alloc_size = sizeof(struct block_jmp);
        break;
    case BLOCK_CMP:
        alloc_size = sizeof(struct block_cmp);
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

void block_insert_first_child(struct block *parent, struct block *child)
{
    child->next = parent->child;
    parent->child = child;
}

void block_add_next(struct block *block, struct block *next)
{
    struct block *walker;

    walker = block;
    while (walker->next)
        walker = walker->next;

    walker->next = next;
}

void block_insert_next(struct block *any_block, struct block *sel_block)
{
    sel_block->next = any_block->next;
    any_block->next = sel_block;
}

const char *block_name(struct block *block)
{
    const char *names[] = {
        "NULL",      "FILE_START", "FUNC",   "PREAMBLE",     "EPILOGUE",
        "LOCAL",     "GLOBAL",     "STORE",  "STORE_RETURN", "STORE_RESULT",
        "STORE_ARG", "LOAD",       "STRING", "CALL",         "ASM",
        "JMP",       "LABEL",      "CMP"};
    return names[block->type];
}

static void string_info(struct block_string *self)
{
    printf(" %d \"%s\"", self->string_id, self->value);
}

static void local_info(struct block_local *self)
{
    printf(" %s %s", type_repr(self->local->type), self->local->name);
}

static void global_info(struct block_global *self)
{
    printf(" %s %s", type_repr(self->local->type), self->local->name);
}

static void value_inline(struct value *val)
{
    if (val->value_type == VALUE_NULL)
        printf("null");
    if (val->value_type == VALUE_IMMEDIATE)
        printf("imm%d(%d)", val->imm_value.width, val->imm_value.value);
    if (val->value_type == VALUE_STRING)
        printf("str_id(%d)", val->string_id_value);
    if (val->value_type == VALUE_LOCAL)
        printf("local(%s +%d)", val->local_value->name, val->local_offset);
    if (val->value_type == VALUE_GLOBAL)
        printf("global(%s +%d)", val->local_value->name, val->local_offset);
    if (val->value_type == VALUE_ADDR)
        printf("addr(%s +%d)", val->local_value->name, val->local_offset);

    if (val->value_type == VALUE_INDEX) {
        value_inline(val->index_var);
        printf("[");
        value_inline(val->index_offset);
        printf("]");
    }

    if (val->value_type == VALUE_OP) {
        value_inline(val->op_value.left);

        switch (val->op_value.type) {
        case OP_ADD:
            printf(" ADD ");
            break;
        case OP_SUB:
            printf(" SUB ");
            break;
        default:
            printf(" OP ");
        }

        value_inline(val->op_value.right);
    }
}

static void store_info(struct block_store *self)
{
    printf(" %s +%d = ", self->var->name, self->store_offset);
    value_inline(&self->value);
}

static void store_return_info(struct block_store *self)
{
    printf(" ");
    value_inline(&self->value);
}

static void store_result_info(struct block_store *self)
{
    printf(" %s", self->var->name);
}

static void store_arg_info(struct block_store_arg *self)
{
    printf(" %s [%d]", self->local->name, self->arg);
}

static void call_info(struct block_call *self)
{
    printf(" %s(", self->call_name);
    for (int i = 0; i < self->n_args; i++) {
        value_inline(&self->args[i]);
        printf(", ");
    }
    printf(")");
}

static void jmp_info(struct block_jmp *self)
{
    printf(" %s", self->dest);
    if (self->type == JMP_EQ)
        printf(" flag");
}

static void cmp_info(struct block_cmp *self)
{
    printf(" ");
    value_inline(&self->left);
    printf(" %s ", self->type == CMP_EQ ? "==" : "!=");
    value_inline(&self->right);
}

static void block_tree_dump_level(struct block *block, int level)
{
    for (int i = 0; i < level; i++)
        fputs("  ", stdout);

    printf("\033[34m%s\033[0m", block_name(block));

    switch (block->type) {
    case BLOCK_FUNC:
        printf(" %s", ((struct block_func *) block)->label);
        break;
    case BLOCK_LOCAL:
        local_info((struct block_local *) block);
        break;
    case BLOCK_GLOBAL:
        global_info((struct block_global *) block);
        break;
    case BLOCK_STORE:
        store_info((struct block_store *) block);
        break;
    case BLOCK_STORE_RETURN:
        store_return_info((struct block_store *) block);
        break;
    case BLOCK_STORE_RESULT:
        store_result_info((struct block_store *) block);
        break;
    case BLOCK_STORE_ARG:
        store_arg_info((struct block_store_arg *) block);
        break;
    case BLOCK_STRING:
        string_info((struct block_string *) block);
        break;
    case BLOCK_CALL:
        call_info((struct block_call *) block);
        break;
    case BLOCK_ASM:
        printf(" \"%s\"", ((struct block_asm *) block)->source);
        break;
    case BLOCK_LABEL:
        printf(" %s", ((struct block_label *) block)->label);
        break;
    case BLOCK_JMP:
        jmp_info((struct block_jmp *) block);
        break;
    case BLOCK_CMP:
        cmp_info((struct block_cmp *) block);
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
