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

    local->type = type_register_resolve(self->types, decl->var_type->name);
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

static void convert_node_into_value(struct compiler *self,
                                    struct block_func *func,
                                    struct value *value, struct node *node)
{
    if (node->type == NODE_LITERAL) {
        convert_literal_into_value(self, value, (struct node_literal *) node);
    }

    else if (node->type == NODE_LABEL) {
        value->value_type = VALUE_LOCAL;
        value->local_value =
            find_local(func, ((struct node_label *) node)->name);

        if (!value->local_value)
            ce(self, node->place, "undeclared variable");
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
    struct node_literal *literal;
    struct block_store *store_block;

    var = (struct node_label *) assign_node->child;
    if (var->head.type == NODE_LITERAL)
        ce(self, var->head.place, "cannot assign to literal value");
    if (var->head.type != NODE_LABEL)
        ce(self, var->head.place, "expected a variable name");

    local = find_local(func, var->name);
    if (!local)
        ce(self, var->head.place, "undeclared variable");

    possible_value = var->head.next;
    if (possible_value->type != NODE_LITERAL)
        ce(self, possible_value->place, "can only assign literal values");

    /* Check if we can assign the value to the variable. */

    literal = (struct node_literal *) possible_value;
    check_compat_assign(self, local, literal);

    /* Create a new BLOCK_STORE instruction, but don't assign it to the parent
       just yet - converting a node into a value may take more steps than just
       one. */

    store_block = block_alloc(NULL, BLOCK_STORE);
    store_block->var = local;

    /* Convert the value node into a `struct value`. */

    convert_node_into_value(self, func, &store_block->value,
                            (struct node *) literal);

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

static struct node_func_decl *find_func_decl(struct compiler *self,
                                             const char *name)
{
    struct node *walker;
    struct node_func_decl *decl;

    walker = self->tree->child;

    while (walker) {
        if (walker->type == NODE_FUNC_DECL) {
            decl = (struct node_func_decl *) walker;
            if (!strcmp(decl->name, name))
                return decl;
        }

        walker = walker->next;
    }

    return NULL;
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
                               struct node_func_decl *decl)
{
    struct value *arg;

    if (call_block->n_args < decl->n_params) {
        ce(self, call_node->call_end_place,
           "missing '%s' argument for function",
           decl->param_names[call_block->n_args]);
    }

    if (call_block->n_args > decl->n_params) {
        ce(self, call_block->args[decl->n_params].place,
           "too many arguments for function");
    }

    // TODO compare the types of arguments to the types of parameters
}

static void compile_call(struct compiler *self, struct block_func *func,
                         struct node *call)
{
    struct node_func_decl *decl;
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

    decl = find_func_decl(self, label->name);

    if (!decl)
        ce(self, label->head.place, "undeclared function");

    /* Allocate space for MAX_ARG arguments. */

    call_block = block_alloc(NULL, BLOCK_CALL);
    call_block->args = ac_alloc(global_ac, sizeof(struct value) * MAX_ARG);
    call_block->call_name = string_copy_z(decl->name);

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

    validate_call_args(self, (struct node_call *) call, call_block, decl);

    block_add_child((struct block *) func, (struct block *) call_block);
}

static void compile_func(struct compiler *self, struct node_func_def *func)
{
    struct block_func *func_block;
    struct node *node_walker;

    func_block = block_alloc(self->file_block, BLOCK_FUNC);
    func_block->label = string_copy_z(func->decl->name);
    func_block->exported = !(func->decl->attrs.flags & ATTR_LOCAL);

    if (!(func->decl->attrs.flags & ATTR_NAKED))
        block_alloc((struct block *) func_block, BLOCK_FUNC_PREAMBLE);

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
        default:
            ceh(self, node_walker->place, "tip: dunno how to compile this",
                "invalid place for this expression");
        }

        node_walker = node_walker->next;
    }

    /* After compiling the function, we probably have gathered some locals,
       which we need to allocate space for. */

    add_local_blocks(func_block);

    if (!(func->decl->attrs.flags & ATTR_NAKED))
        block_alloc((struct block *) func_block, BLOCK_FUNC_EPILOGUE);
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

void compiler_compile(struct compiler *self)
{
    struct node *func_walker;

    self->file_block = block_alloc(NULL, BLOCK_FILE);
    self->types = ac_alloc(global_ac, sizeof(struct type_register));
    self->strings = NULL;
    self->n_strings = 0;

    add_builtin_types(self);

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
