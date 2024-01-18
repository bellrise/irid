/* Leaf compiler
   Copyright (c) 2024 bellrise */

#include "compiler.h"

static struct func_sig *alloc_func_sig(struct compiler *self)
{
    struct func_sig *sig;

    sig = ac_alloc(global_ac, sizeof(struct func_sig));
    self->funcs = ac_realloc(global_ac, self->funcs,
                             sizeof(struct func_sig *) * (self->n_funcs + 1));
    self->funcs[self->n_funcs++] = sig;

    return sig;
}

static void func_sig_add_param_type(struct func_sig *func, struct type *type)
{
    func->param_types =
        ac_realloc(global_ac, func->param_types,
                   sizeof(struct type *) * (func->n_params + 1));
    func->param_types[func->n_params++] = type;
}

void compile_func_decl(struct compiler *self, struct node_func_decl *func_decl)
{
    struct func_sig *func;
    struct type *type;

    for (int i = 0; i < self->n_funcs; i++) {
        if (!strcmp(self->funcs[i]->name, func_decl->name))
            ce(self, func_decl->head.place, "this function is already defined");
    }

    func = alloc_func_sig(self);
    func->name = string_copy_z(func_decl->name);
    func->decl = func_decl;

    if (func_decl->return_type)
        func->return_type = make_real_type(self, func_decl->return_type);

    for (int i = 0; i < func_decl->n_params; i++) {
        type = make_real_type(self, func_decl->param_types[i]);
        func_sig_add_param_type(func, type);
    }
}

static void add_local_blocks(struct block_func *func)
{
    struct block_local *local_block;
    struct block *insert_after;

    /* We want to insert all the LOCAL blocks after the PREAMBLE. */

    insert_after = func->head.child;

    for (int i = 0; i < func->n_locals; i++) {
        local_block = block_alloc(NULL, BLOCK_LOCAL);
        local_block->local = func->locals[i];
        local_block->local->local_block = local_block;
        block_insert_next(insert_after, (struct block *) local_block);
    }
}

static void func_store_arguments(struct compiler *self,
                                 struct node_func_decl *func_decl,
                                 struct block_func *func_block)
{
    struct block_store_arg *store_block;
    struct func_sig *func_sig;
    struct local *arg_local;

    func_sig = find_func(self, func_decl->name);

    for (int i = 0; i < func_sig->n_params; i++) {
        arg_local = alloc_local(func_block);
        arg_local->type = func_sig->param_types[i];
        arg_local->name = string_copy_z(func_decl->param_names[i]);
        arg_local->decl = NULL;

        store_block = block_alloc((struct block *) func_block, BLOCK_STORE_ARG);
        store_block->arg = i;
        store_block->local = arg_local;
    }
}

void compile_func(struct compiler *self, struct node_func_def *func)
{
    struct block_func *func_block;
    struct block_label *exit_label;
    struct node *node_walker;
    int node_type;

    func_block = block_alloc(self->file_block, BLOCK_FUNC);
    func_block->label = string_copy_z(func->decl->name);
    func_block->exported = !(func->decl->attrs.flags & ATTR_LOCAL);

    if (func->decl->return_type)
        func_block->return_type = make_real_type(self, func->decl->return_type);

    if (!(func->decl->attrs.flags & ATTR_NAKED))
        block_alloc((struct block *) func_block, BLOCK_PREAMBLE);

    func_store_arguments(self, func->decl, func_block);

    node_walker = func->head.child;
    while (node_walker) {
        place_node(self, func_block, node_walker);
        node_walker = node_walker->next;
    }

    /* After compiling the function, we probably have gathered some locals,
       which we need to allocate space for. */

    add_local_blocks(func_block);

    /* If the function has a declared return value, and the last node is not a
       return node, issue a warning. */

    if (func->decl->return_type) {
        node_walker = func->head.child;
        if (!node_walker) {
            node_type = NODE_NULL;
        } else {
            while (node_walker->next)
                node_walker = node_walker->next;
            node_type = node_walker->type;
        }

        if (node_type != NODE_RETURN) {
            cn(self, func->decl->return_type->place, "%s returns %s",
               func->decl->name, parsed_type_repr(func->decl->return_type));
            cw(self, func->decl->head.place,
               "missing return statement at the end of this function");
        }
    }

    exit_label = block_alloc((struct block *) func_block, BLOCK_LABEL);
    exit_label->label = string_copy_z("E");

    if (!(func->decl->attrs.flags & ATTR_NAKED))
        block_alloc((struct block *) func_block, BLOCK_EPILOGUE);

    /* Walk through the locals to check if there are any unused variables.
     */

    if (self->opts->w_unused_var) {
        for (int i = 0; i < func_block->n_locals; i++) {
            if (func_block->locals[i]->used)
                continue;
            if (!func_block->locals[i]->decl)
                continue;

            cw(self, func_block->locals[i]->decl->head.place,
               "unused variable");
        }
    }
}
