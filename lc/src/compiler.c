/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ce(struct compiler *self, struct tok *place, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    source_error(self->tokens, place->pos, place->len, 0, NULL, NULL, "error",
                 "\033[31m", fmt, args);
    va_end(args);
    exit(1);
}

static void cw(struct compiler *self, struct tok *place, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    source_error(self->tokens, place->pos, place->len, 0, NULL, NULL, "warning",
                 "\033[35m", fmt, args);
    va_end(args);
}

static void ceh(struct compiler *self, struct tok *place, const char *help_msg,
                const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    source_error(self->tokens, place->pos, place->len, 0, NULL, help_msg,
                 "error", "\033[31m", fmt, args);
    va_end(args);
    exit(1);
}

static void assign_error(struct compiler *self, struct local *local,
                         struct node_literal *literal)
{
    /* Actual error. */

    source_error_nv(
        self->tokens, literal->head.place->pos, literal->head.place->len, 0,
        NULL, NULL, "error", "\033[31m",
        "type mismatch: cannot assign this value to '%s'", local->name);

    source_error_nv(self->tokens, local->decl->head.place->pos,
                    local->decl->head.place->len, 0, NULL,
                    "tip: change the type of the variable", NULL, "\033[35m",
                    "declared here", local->name);
}

static struct type *make_real_type(struct compiler *self,
                                   struct parsed_type *parsed_type)
{
    struct type *base_type;

    base_type = type_register_resolve(self->types, parsed_type->name);
    if (!base_type)
        ce(self, parsed_type->place, "unknown type");

    if (parsed_type->is_pointer) {
        return (struct type *) type_register_add_pointer(self->types,
                                                         base_type);
    }

    return base_type;
}

static struct local *alloc_local(struct block_func *func)
{
    struct local *local;

    local = ac_alloc(global_ac, sizeof(*local));
    func->locals = ac_realloc(global_ac, func->locals,
                              sizeof(struct local *) * (func->n_locals + 1));
    func->locals[func->n_locals++] = local;

    return local;
}

static struct local *find_local(struct block_func *func, const char *name)
{
    for (int i = 0; i < func->n_locals; i++) {
        if (!strcmp(func->locals[i]->name, name))
            return func->locals[i];
    }

    return NULL;
}

static void compile_var_decl(struct compiler *self, struct block_func *func,
                             struct node_var_decl *decl)
{
    struct local *local;

    local = alloc_local(func);

    local->type = make_real_type(self, decl->var_type);
    if (!local->type)
        ce(self, decl->var_type->place, "unknown type");
    local->name = string_copy_z(decl->name);
    local->decl = decl;
}

static void check_compat_assign(struct compiler *self, struct local *local,
                                struct node_literal *literal)
{
    if (local->type->type == TYPE_INTEGER) {
        if (literal->type == LITERAL_INTEGER)
            return;
    }

    assign_error(self, local, literal);
}

static int make_string_id(struct compiler *self, const char *string)
{
    struct block_string *string_block;

    string_block = block_alloc(NULL, BLOCK_STRING);
    string_block->value = string_copy_z(string);
    string_block->string_id = self->n_strings + 1;

    /* Append the string block to the interal string table, which will get added
       to the top-level FILE block after finishing the rest. */

    self->strings = ac_realloc(global_ac, self->strings,
                               sizeof(struct block *) * (self->n_strings + 1));
    self->strings[self->n_strings++] = string_block;

    return string_block->string_id;
}

static void convert_literal_into_value(struct compiler *self,
                                       struct value *value,
                                       struct node_literal *literal)
{
    if (literal->type == LITERAL_INTEGER) {
        value->value_type = VALUE_IMMEDIATE;
        value->imm_value.value = literal->integer_value;
        value->imm_value.width = literal->integer_value < 256 ? 8 : 16;
    }

    else if (literal->type == LITERAL_STRING) {
        value->value_type = VALUE_STRING;
        value->string_id_value = make_string_id(self, literal->string_value);
    }

    else {
        ce(self, literal->head.place,
           "don't know how to convert this into a value");
    }
}

static struct func_sig *find_func(struct compiler *self, char *name)
{
    for (int i = 0; i < self->n_funcs; i++) {
        if (!strcmp(self->funcs[i]->name, name))
            return self->funcs[i];
    }

    return NULL;
}

static void compile_call(struct compiler *self, struct block_func *func,
                         struct node *call);

static void convert_call_node_into_value(struct compiler *self,
                                         struct block_func *func,
                                         struct value *value, struct node *node)
{

    struct local *result_local;
    struct func_sig *func_sig;
    struct node_label *func_name;
    struct block_store *store_block;

    /* Allocate a new local to store our result. */

    func_name = (struct node_label *) node->child;
    if (func_name->head.type != NODE_LABEL)
        ce(self, func_name->head.place, "expected function name here");

    func_sig = find_func(self, func_name->name);
    if (!func_sig)
        ce(self, func_name->head.place, "undeclared function");

    if (type_is_null(func_sig->return_type)) {
        ce(self, func_name->head.place, "%s does not return any value",
           func_name->name);
    }

    result_local = alloc_local(func);
    result_local->name = ac_alloc(global_ac, 8);
    result_local->type = func_sig->return_type;

    snprintf(result_local->name, 8, "%%%d", func->var_index++);

    compile_call(self, func, node);

    store_block = block_alloc((struct block *) func, BLOCK_STORE_RESULT);
    store_block->var = result_local;

    value->value_type = VALUE_LOCAL;
    value->local_value = result_local;
}

static void convert_node_into_value(struct compiler *self,
                                    struct block_func *func,
                                    struct value *value, struct node *node)
{
    struct node *left;
    struct node *right;

    if (node->type == NODE_LITERAL) {
        convert_literal_into_value(self, value, (struct node_literal *) node);
    }

    else if (node->type == NODE_LABEL) {
        value->value_type = VALUE_LOCAL;
        value->local_value =
            find_local(func, ((struct node_label *) node)->name);
        if (!value->local_value)
            ce(self, node->place, "undeclared variable");

        value->local_value->used = true;
    }

    else if (node->type == NODE_ADD) {
        value->value_type = VALUE_OP;

        left = node->child;
        if (!left)
            ce(self, node->place, "missing left-hand value");
        right = left->next;
        if (!right)
            ce(self, node->place, "missing right-hand value");

        value->op_value.type = OP_ADD;
        value->op_value.left = ac_alloc(global_ac, sizeof(struct value));
        value->op_value.right = ac_alloc(global_ac, sizeof(struct value));

        convert_node_into_value(self, func, value->op_value.left, left);
        convert_node_into_value(self, func, value->op_value.right, right);
    }

    else if (node->type == NODE_CALL) {
        convert_call_node_into_value(self, func, value, node);
    }

    else {
        ce(self, node->place, "cannot convert this into a value");
    }
}

static void compile_assign(struct compiler *self, struct block_func *func,
                           struct node *assign_node)
{
    struct local *local;
    struct node_label *var;
    struct node *possible_value;
    struct block_store *store_block;

    var = (struct node_label *) assign_node->child;
    if (var->head.type == NODE_LITERAL)
        ce(self, var->head.place, "cannot assign to literal value");
    if (var->head.type != NODE_LABEL)
        ce(self, var->head.place, "expected a variable name");

    local = find_local(func, var->name);
    if (!local)
        ce(self, var->head.place, "undeclared variable");
    local->used = true;

    /* Create a new BLOCK_STORE instruction, but don't assign it to the parent
       just yet - converting a node into a value may take more steps than just
       one. */

    store_block = block_alloc(NULL, BLOCK_STORE);
    store_block->var = local;

    possible_value = var->head.next;
    convert_node_into_value(self, func, &store_block->value, possible_value);

    /* Check if we can assign the value to the variable. */

    // TODO: add this back
    // check_compat_assign(self, local, literal);

    /* Convert the value node into a `struct value`. */

    block_add_child((struct block *) func, (struct block *) store_block);
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

static void compile_asm_call(struct compiler *self, struct block_func *func,
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

static void validate_call_args(struct compiler *self,
                               struct node_call *call_node,
                               struct block_call *call_block,
                               struct func_sig *func)
{
    if (call_block->n_args < func->n_params) {
        ce(self, call_node->call_end_place,
           "missing '%s' argument for function",
           func->decl->param_names[call_block->n_args]);
    }

    if (call_block->n_args > func->n_params) {
        ce(self, call_block->args[func->n_params].place,
           "too many arguments for function");
    }

    // TODO compare the types of arguments to the types of parameters
}

static void compile_call(struct compiler *self, struct block_func *func,
                         struct node *call)
{
    struct func_sig *func_sig;
    struct node_label *label;
    struct block_call *call_block;
    struct node *arg;

    label = (struct node_label *) call->child;
    if (label->head.type == NODE_LITERAL)
        ce(self, label->head.place, "cannot call literal value");
    if (label->head.type != NODE_LABEL)
        ce(self, label->head.place, "expected function name");

    if (!strcmp(label->name, "__asm")) {
        compile_asm_call(self, func, call);
        return;
    }

    func_sig = find_func(self, label->name);

    if (!func_sig)
        ce(self, label->head.place, "undeclared function");

    /* Allocate space for MAX_ARG arguments. */

    call_block = block_alloc(NULL, BLOCK_CALL);
    call_block->args = ac_alloc(global_ac, sizeof(struct value) * MAX_ARG);
    call_block->call_name = string_copy_z(func_sig->name);

    arg = label->head.next;
    while (arg) {
        if (call_block->n_args == MAX_ARG) {
            ce(self, arg->place,
               "cannot call function with more than %d arguments", MAX_ARG);
        }

        convert_node_into_value(self, func,
                                &call_block->args[call_block->n_args++], arg);
        call_block->args[call_block->n_args - 1].place = arg->place;
        arg = arg->next;
    }

    validate_call_args(self, (struct node_call *) call, call_block, func_sig);

    block_add_child((struct block *) func, (struct block *) call_block);
}

static void compile_return(struct compiler *self, struct block_func *func_block,
                           struct node *return_node)
{
    struct block_store *return_block;
    struct block_jmp *exit_jmp;
    struct node *val;

    /* With a return type. */

    if (!type_is_null(func_block->return_type)) {
        if (!return_node->child)
            ce(self, return_node->place, "missing return value");
        val = return_node->child;
    }

    /* Without a return type. */
    else {
        if (return_node->child) {
            ce(self, return_node->child->place,
               "this function cannot not return any value");
        }
        val = NULL;
    }

    if (val) {
        return_block = block_alloc(NULL, BLOCK_STORE_RETURN);
        convert_node_into_value(self, func_block, &return_block->value, val);
        block_add_child((struct block *) func_block,
                        (struct block *) return_block);
    }

    exit_jmp = block_alloc((struct block *) func_block, BLOCK_JMP);
    exit_jmp->dest = string_copy_z("E");
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

static void compile_func(struct compiler *self, struct node_func_def *func)
{
    struct block_func *func_block;
    struct block_label *exit_label;
    struct node *node_walker;

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

        switch (node_walker->type) {
        case NODE_VAR_DECL:
            compile_var_decl(self, func_block,
                             (struct node_var_decl *) node_walker);
            break;
        case NODE_ASSIGN:
            compile_assign(self, func_block, node_walker);
            break;
        case NODE_CALL:
            compile_call(self, func_block, node_walker);
            break;
        case NODE_RETURN:
            compile_return(self, func_block, node_walker);
            break;
        default:
            ceh(self, node_walker->place, "tip: dunno how to compile this",
                "invalid place for this expression");
        }

        node_walker = node_walker->next;
    }

    /* After compiling the function, we probably have gathered some locals,
       which we need to allocate space for. */

    add_local_blocks(func_block);

    exit_label = block_alloc((struct block *) func_block, BLOCK_LABEL);
    exit_label->label = string_copy_z("E");

    if (!(func->decl->attrs.flags & ATTR_NAKED))
        block_alloc((struct block *) func_block, BLOCK_EPILOGUE);

    /* Walk through the locals to check if there are any unused variables. */

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

static void add_builtin_types(struct compiler *self)
{
    struct type_integer *type;

    type = type_alloc(self->types, TYPE_INTEGER);
    type->head.name = string_copy("int", 3);
    type->bit_width = 16;

    type = type_alloc(self->types, TYPE_INTEGER);
    type->head.name = string_copy("char", 4);
    type->bit_width = 8;
}

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

static void compile_func_decl(struct compiler *self,
                              struct node_func_decl *func_decl)
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

void compiler_compile(struct compiler *self)
{
    struct node *func_walker;

    self->file_block = block_alloc(NULL, BLOCK_FILE);
    self->types = ac_alloc(global_ac, sizeof(struct type_register));
    self->strings = NULL;
    self->n_strings = 0;
    self->funcs = NULL;
    self->n_funcs = 0;

    add_builtin_types(self);

    /* First, collect all function declarations. */

    func_walker = self->tree->child;
    while (func_walker) {
        if (func_walker->type == NODE_FUNC_DECL)
            compile_func_decl(self, (struct node_func_decl *) func_walker);
        func_walker = func_walker->next;
    }

    /* Walk through the tree again to compile all function definitions. */

    func_walker = self->tree->child;
    while (func_walker) {
        if (func_walker->type == NODE_FUNC_DEF)
            compile_func(self, (struct node_func_def *) func_walker);

        func_walker = func_walker->next;
    }

    /* We need to remmeber to add all the STRING blocks to the file. */

    for (int i = 0; i < self->n_strings; i++) {
        block_insert_first_child(self->file_block,
                                 (struct block *) self->strings[i]);
    }

    block_tree_dump(self->file_block);
}
