/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdio.h>
#include <stdlib.h>

void *node_alloc(struct node *parent, int type)
{
    struct node *node;
    size_t alloc_size;

    switch (type) {
    case NODE_FUNC_DECL:
        alloc_size = sizeof(struct node_func_decl);
        break;
    case NODE_FUNC_DEF:
        alloc_size = sizeof(struct node_func_def);
        break;
    case NODE_VAR_DECL:
        alloc_size = sizeof(struct node_var_decl);
        break;
    case NODE_TYPE_DECL:
        alloc_size = sizeof(struct node_type_decl);
        break;
    case NODE_LABEL:
        alloc_size = sizeof(struct node_label);
        break;
    case NODE_LITERAL:
        alloc_size = sizeof(struct node_literal);
        break;
    case NODE_CALL:
        alloc_size = sizeof(struct node_call);
        break;
    default:
        alloc_size = sizeof(struct node);
    }

    node = ac_alloc(global_ac, alloc_size);
    node->type = type;
    node->parent = parent;

    if (parent)
        node_add_child(parent, node);

    return node;
}

void node_add_child(struct node *parent, struct node *child)
{
    struct node *walker;

    if (!parent->child) {
        parent->child = child;
        return;
    }

    walker = parent->child;
    while (walker->next)
        walker = walker->next;

    walker->next = child;
}

const char *node_name(struct node *node)
{
    const char *names[] = {
        "NULL",   "FILE",   "FUNC_DECL", "FUNC_DEF", "TYPE_DECL", "VAR_DECL",
        "ASSIGN", "ADD",    "SUB",       "MUL",      "DIV",       "MOD",
        "CMPEQ",  "CMPNEQ", "CALL",      "LABEL",    "LITERAL",   "RETURN"};
    return names[node->type];
}

static void func_decl_info(struct node_func_decl *self)
{
    printf(" \033[32m%s\033[0m(", self->name);

    for (int i = 0; i < self->n_params - 1; i++) {
        parsed_type_dump_inline(self->param_types[i]);
        printf(" %s, ", self->param_names[i]);
    }

    if (self->n_params) {
        parsed_type_dump_inline(self->param_types[self->n_params - 1]);
        printf(" %s", self->param_names[self->n_params - 1]);
    }

    printf(") -> ");
    parsed_type_dump_inline(self->return_type);
}

static void var_decl_info(struct node_var_decl *self)
{
    fputc(' ', stdout);
    parsed_type_dump_inline(self->var_type);
    printf(" \033[32m%s\033[0m", self->name);
}

static void label_info(struct node_label *self)
{
    printf(" \033[32m%s\033[0m", self->name);
}

static void literal_info(struct node_literal *self)
{
    printf("\033[33m");

    if (self->type == LITERAL_INTEGER)
        printf(" int(%d)", self->integer_value);
    if (self->type == LITERAL_STRING)
        printf(" str(\"%s\")", self->string_value);

    printf("\033[0m");
}

static void type_decl_info(struct node_type_decl *self)
{
    printf(" \033[31m%s\033[0m { ", self->typename);

    for (int i = 0; i < self->n_fields; i++) {
        parsed_type_dump_inline(self->fields[i]->parsed_type);
        printf(" %s; ", self->fields[i]->name);
    }

    printf("}");
}

static void node_tree_dump_level(struct node *node, int level)
{
    for (int i = 0; i < level; i++)
        fputs("  ", stdout);

    printf("\033[34m%s\033[0m", node_name(node));

    switch (node->type) {
    case NODE_FUNC_DECL:
        func_decl_info((struct node_func_decl *) node);
        break;
    case NODE_FUNC_DEF:
        func_decl_info(((struct node_func_def *) node)->decl);
        break;
    case NODE_VAR_DECL:
        var_decl_info(((struct node_var_decl *) node));
        break;
    case NODE_LABEL:
        label_info(((struct node_label *) node));
        break;
    case NODE_LITERAL:
        literal_info(((struct node_literal *) node));
        break;
    case NODE_TYPE_DECL:
        type_decl_info((struct node_type_decl *) node);
        break;
    };

    fputc('\n', stdout);

    if (node->child)
        node_tree_dump_level(node->child, level + 1);

    if (node->next)
        node_tree_dump_level(node->next, level);
}

void node_tree_dump(struct node *node)
{
    node_tree_dump_level(node, 0);
}
