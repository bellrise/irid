/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct type *resolve_value_type(struct compiler *self,
                                       struct value *val);

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

static struct type *make_real_type(struct compiler *self,
                                   struct parsed_type *parsed_type)
{
    struct type *base_type;

    base_type = type_register_resolve(self->types, parsed_type->name);
    if (!base_type)
        ce(self, parsed_type->place, "unknown type");

    if (parsed_type->is_pointer) {
        base_type =
            (struct type *) type_register_add_pointer(self->types, base_type);
    }

    if (parsed_type->count) {
        base_type = (struct type *) type_register_add_array(
            self->types, base_type, parsed_type->count);
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

static struct local *alloc_global(struct compiler *self)
{
    struct local *global;

    global = ac_alloc(global_ac, sizeof(*global));
    self->globals = ac_realloc(global_ac, self->globals,
                               sizeof(struct local *) * (self->n_globals + 1));
    self->globals[self->n_globals++] = global;

    return global;
}

static struct local *find_local(struct block_func *func, const char *name)
{
    for (int i = 0; i < func->n_locals; i++) {
        if (!strcmp(func->locals[i]->name, name))
            return func->locals[i];
    }

    return NULL;
}

static struct local *find_global(struct compiler *self, const char *name)
{
    for (int i = 0; i < self->n_globals; i++) {
        if (!strcmp(self->globals[i]->name, name))
            return self->globals[i];
    }

    return NULL;
}

static void compile_var_decl(struct compiler *self, struct block_func *func,
                             struct node_var_decl *decl)
{
    struct local *local;

    if (find_local(func, decl->name))
        ce(self, decl->head.place, "%s is already declared", decl->name);

    local = alloc_local(func);

    local->type = make_real_type(self, decl->var_type);
    if (!local->type)
        ce(self, decl->var_type->place, "unknown type");
    local->name = string_copy_z(decl->name);
    local->decl = decl;
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

static void convert_field_into_value(struct compiler *self,
                                     struct block_func *func,
                                     struct value *value, struct node *node)
{
    struct local *local;
    struct node_label *var;
    struct node_label *field;
    struct type *field_type;
    struct type_struct *var_type;
    bool deref_needed;

    value->value_type = VALUE_LOCAL;

    var = (struct node_label *) node->child;
    if (var->head.type != NODE_LABEL)
        ce(self, var->head.place, "expected var name");

    field = (struct node_label *) var->head.next;
    if (field->head.type != NODE_LABEL)
        ce(self, var->head.place, "missing field name");

    local = find_local(func, var->name);
    if (!local)
        local = find_global(self, var->name);

    if (!local)
        ce(self, var->head.place, "undeclared variale");

    value->local_value = local;

    deref_needed = false;
    var_type = (struct type_struct *) local->type;

    if (local->type->type == TYPE_POINTER) {
        var_type = (struct type_struct *) ((struct type_pointer *) local->type)
                       ->base_type;
        deref_needed = true;
    }

    if (var_type->head.type != TYPE_STRUCT)
        ce(self, value->place, "cannot access field of non-strut type");

    field_type = type_struct_find_field(var_type, field->name);
    if (!field_type)
        ce(self, field->head.place, "no field named %s in struct", field->name);

    value->local_offset += type_struct_field_offset(var_type, field->name);
    value->field_type = field_type;
    value->deref = deref_needed;
}

static void convert_node_into_value(struct compiler *self,
                                    struct block_func *func,
                                    struct value *value, struct node *node)
{
    struct node *left;
    struct node *right;
    struct type *type;

    if (node->type == NODE_LITERAL) {
        convert_literal_into_value(self, value, (struct node_literal *) node);
    }

    else if (node->type == NODE_LABEL) {
        /* First, find local. */
        value->value_type = VALUE_LOCAL;
        value->local_value =
            find_local(func, ((struct node_label *) node)->name);

        if (!value->local_value) {
            /* Otherwise, find a global. */
            value->value_type = VALUE_GLOBAL;
            value->local_value =
                find_global(self, ((struct node_label *) node)->name);
        }

        if (!value->local_value)
            ce(self, node->place, "undeclared variable");

        value->local_value->used = true;
    }

    else if (node->type == NODE_CALL) {
        convert_call_node_into_value(self, func, value, node);
    }

    else if (node->type == NODE_FIELD) {
        convert_field_into_value(self, func, value, node);
    }

    else if (node->type == NODE_ADDR) {
        value->value_type = VALUE_ADDR;
        value->local_value =
            find_local(func, ((struct node_label *) node)->name);
        if (!value->local_value)
            ce(self, node->place, "undeclared variable");

        value->local_value->used = true;
    }

    else if (node->type == NODE_INDEX) {
        value->value_type = VALUE_INDEX;
        value->index_offset = ac_alloc(global_ac, sizeof(struct value));
        value->index_var = ac_alloc(global_ac, sizeof(struct value));

        left = node->child;
        right = left->next;

        convert_node_into_value(self, func, value->index_var, left);

        type = resolve_value_type(self, value->index_var);
        if (type->type != TYPE_POINTER)
            ce(self, node->place, "cannot index into non-pointer variable");

        value->index_elem_size =
            type_size(((struct type_pointer *) type)->base_type);

        convert_node_into_value(self, func, value->index_offset, right);
    }

    else {
        switch (node->type) {
        case NODE_ADD:
        case NODE_SUB:
        case NODE_MUL:
        case NODE_CMPEQ:
        case NODE_CMPNEQ:
            value->value_type = VALUE_OP;

            left = node->child;
            if (!left)
                ce(self, node->place, "missing left-hand value");
            right = left->next;
            if (!right)
                ce(self, node->place, "missing right-hand value");

            if (node->type == NODE_ADD)
                value->op_value.type = OP_ADD;
            if (node->type == NODE_SUB)
                value->op_value.type = OP_SUB;
            if (node->type == NODE_MUL)
                value->op_value.type = OP_MUL;
            if (node->type == NODE_CMPEQ)
                value->op_value.type = OP_CMPEQ;
            if (node->type == NODE_CMPNEQ)
                value->op_value.type = OP_CMPNEQ;

            value->op_value.left = ac_alloc(global_ac, sizeof(struct value));
            value->op_value.right = ac_alloc(global_ac, sizeof(struct value));

            convert_node_into_value(self, func, value->op_value.left, left);
            convert_node_into_value(self, func, value->op_value.right, right);
            break;

        default:
            ce(self, node->place, "cannot convert this into a value");
        }
    }

    if (node->cast_into)
        value->cast_type = make_real_type(self, node->cast_into);
}

static void compile_assign_field(struct compiler *self, struct block_func *func,
                                 struct node *assign_node,
                                 struct node_field *field_node)
{
    struct local *local;
    struct node_label *var;
    struct node_label *field;
    struct type_struct *var_type;
    struct block_store *store_block;
    struct node *possible_value;
    int offset;

    var = (struct node_label *) field_node->head.child;
    if (var->head.type == NODE_LITERAL)
        ce(self, var->head.place, "cannot access field of literal");
    if (var->head.type != NODE_LABEL)
        ce(self, var->head.place, "expected a variable name");

    local = find_local(func, var->name);
    if (!local)
        local = find_global(self, var->name);

    if (!local)
        ce(self, var->head.place, "undeclared variable");

    local->used = true;

    field = (struct node_label *) var->head.next;
    if (field->head.type != NODE_LABEL)
        ce(self, field->head.place, "expected a field name");

    /* Storing a value in a field is simply a STORE instruction with a specific
       offset from the base pointer. */

    var_type = (struct type_struct *) local->type;
    if (var_type->head.type != TYPE_STRUCT)
        ce(self, field_node->head.place,
           "cannot access field of non-struct type");

    if (!type_struct_find_field(var_type, field->name)) {
        ce(self, field_node->field_tok, "no field named %s in %s", field->name,
           var_type->head.name);
    }

    offset = type_struct_field_offset(var_type, field->name);

    store_block = block_alloc(NULL, BLOCK_STORE);
    store_block->var = local;
    store_block->store_offset = offset;

    possible_value = assign_node->child->next;
    convert_node_into_value(self, func, &store_block->value, possible_value);

    block_add_child((struct block *) func, (struct block *) store_block);
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

    if (var->head.type != NODE_LABEL && var->head.type != NODE_FIELD) {
        ce(self, var->head.place, "expected a variable or structure field");
    }

    if (var->head.type == NODE_FIELD) {
        return compile_assign_field(self, func, assign_node,
                                    (struct node_field *) var);
    }

    local = find_local(func, var->name);
    if (!local)
        local = find_global(self, var->name);

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

static struct type *resolve_value_type(struct compiler *self, struct value *val)
{
    struct type *resolved_type;

    if (val->cast_type)
        resolved_type = val->cast_type;

    switch (val->value_type) {
    case VALUE_IMMEDIATE:
        resolved_type = type_register_resolve(self->types, "int");
        break;

    case VALUE_STRING:
        resolved_type = (struct type *) type_register_add_pointer(
            self->types, type_register_resolve(self->types, "char"));
        break;

    case VALUE_GLOBAL:
    case VALUE_LOCAL:
        if (val->field_type)
            resolved_type = val->field_type;
        else

            resolved_type = val->local_value->type;
        break;

    case VALUE_ADDR:
        resolved_type = (struct type *) type_register_add_pointer(
            self->types, val->local_value->type);
        break;

    case VALUE_INDEX:
        resolved_type = resolve_value_type(self, val->index_var);
        if (!resolved_type)
            return NULL;

        if (resolved_type->type != TYPE_POINTER)
            die("cannot resolve_value_type of non-pointer VALUE_INDEX");

        resolved_type = ((struct type_pointer *) resolved_type)->base_type;
        break;

    case VALUE_OP:
        resolved_type = resolve_value_type(self, val->op_value.left);
        break;

    default:
        return NULL;
    }

    /* Possibly decay the array into a pointer. */

    if (val->decay_into_pointer && resolved_type->type == TYPE_ARRAY) {
        resolved_type = (struct type *) type_register_add_pointer(
            self->types, ((struct type_array *) resolved_type)->base_type);
    }

    return resolved_type;
}

static int min(int a, int b)
{
    return a < b ? a : b;
}

static void decay_arrays_to_pointers(struct compiler *self,
                                     struct block_call *call_block,
                                     struct func_sig *func)
{
    struct type_array *array_type;
    struct type_pointer *pointer_type;
    struct type *arg_type;
    struct value *arg;
    int check_args;

    check_args = min(call_block->n_args, func->n_params);

    for (int i = 0; i < check_args; i++) {
        arg = &call_block->args[i];
        arg_type = resolve_value_type(self, arg);

        if (arg_type->type != TYPE_ARRAY
            || func->param_types[i]->type != TYPE_POINTER) {
            continue;
        }

        array_type = (struct type_array *) arg_type;
        pointer_type = (struct type_pointer *) func->param_types[i];

        if (type_cmp(array_type->base_type, pointer_type->base_type))
            arg->decay_into_pointer = true;
    }
}

static void validate_call_args(struct compiler *self,
                               struct node_call *call_node,
                               struct block_call *call_block,
                               struct func_sig *func)
{
    struct value *arg;
    struct type *arg_type;

    if (call_block->n_args < func->n_params) {
        cw(self, func->decl->param_places[call_block->n_args], "here");
        ce(self, call_node->call_end_place,
           "missing '%s' argument for function",
           func->decl->param_names[call_block->n_args]);
    }

    if (!func->decl->is_variadic && call_block->n_args > func->n_params) {
        ce(self, call_block->args[func->n_params].place,
           "too many arguments for function");
    }

    for (int i = 0; i < func->n_params; i++) {
        arg = &call_block->args[i];
        arg_type = resolve_value_type(self, arg);

        if (!arg_type) {
            if (arg->value_type == VALUE_LOCAL
                || arg->value_type == VALUE_GLOBAL) {
                cw(self, arg->local_value->decl->head.place, "declared here");
            }

            ce(self, call_block->args[i].place, "unresolved type of argument");
        }

        if (!type_cmp(func->param_types[i], arg_type)) {
            cw(self, func->decl->param_places[i], "here");
            ce(self, call_block->args[i].place,
               "type mismatch: function expected %s, got %s",
               type_repr(func->param_types[i]), type_repr(arg_type));
        }
    }
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

    decay_arrays_to_pointers(self, call_block, func_sig);
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

static void place_node(struct compiler *self, struct block_func *func_node,
                       struct node *this_node);

static void compile_if(struct compiler *self, struct block_func *func_block,
                       struct node *if_node)
{
    struct block_jmp *jmp_yes_block;
    struct block_jmp *jmp_no_block;
    struct block_label *yes_label;
    struct block_label *no_label;
    struct block_cmp *cmp_block;
    struct node *cmp_node;
    struct node *left;
    struct node *right;
    struct node *walker;

    /* Create the labels. */

    yes_label = block_alloc(NULL, BLOCK_LABEL);
    no_label = block_alloc(NULL, BLOCK_LABEL);

    yes_label->label = ac_alloc(global_ac, 8);
    no_label->label = ac_alloc(global_ac, 8);

    snprintf(yes_label->label, 8, "L%d", func_block->label_index++);
    snprintf(no_label->label, 8, "L%d", func_block->label_index++);

    /* Run the comparison and store the result. */

    cmp_block = block_alloc(NULL, BLOCK_CMP);

    cmp_node = if_node->child;
    if (cmp_node->type != NODE_CMPEQ && cmp_node->type != NODE_CMPNEQ)
        ce(self, cmp_node->place, "expected comparison in if condition");

    left = cmp_node->child;
    right = left->next;

    convert_node_into_value(self, func_block, &cmp_block->left, left);
    convert_node_into_value(self, func_block, &cmp_block->right, right);

    jmp_yes_block = block_alloc(NULL, BLOCK_JMP);
    jmp_yes_block->type = cmp_node->type == NODE_CMPEQ ? JMP_EQ : JMP_NEQ;
    jmp_yes_block->dest = yes_label->label;

    jmp_no_block = block_alloc(NULL, BLOCK_JMP);
    jmp_no_block->type = JMP_ALWAYS;
    jmp_no_block->dest = no_label->label;

    block_add_child((struct block *) func_block, (struct block *) cmp_block);
    block_add_child((struct block *) func_block,
                    (struct block *) jmp_yes_block);
    block_add_child((struct block *) func_block, (struct block *) jmp_no_block);
    block_add_child((struct block *) func_block, (struct block *) yes_label);

    walker = if_node->child->next;

    while (walker) {
        place_node(self, func_block, walker);
        walker = walker->next;
    }

    block_add_child((struct block *) func_block, (struct block *) no_label);
}

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
    store_first->var = index_local;
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
    inc_index->var = index_local;
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

static void compile_loop(struct compiler *self, struct block_func *func_block,
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

static void place_node(struct compiler *self, struct block_func *func_node,
                       struct node *this_node)
{
    switch (this_node->type) {
    case NODE_VAR_DECL:
        compile_var_decl(self, func_node, (struct node_var_decl *) this_node);
        break;
    case NODE_ASSIGN:
        compile_assign(self, func_node, this_node);
        break;
    case NODE_CALL:
        compile_call(self, func_node, this_node);
        break;
    case NODE_RETURN:
        compile_return(self, func_node, this_node);
        break;
    case NODE_IF:
        compile_if(self, func_node, this_node);
        break;
    case NODE_LOOP:
        compile_loop(self, func_node, (struct node_loop *) this_node);
        break;
    default:
        ceh(self, this_node->place, "tip: dunno how to compile this",
            "invalid place for this expression");
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
        place_node(self, func_block, node_walker);
        node_walker = node_walker->next;
    }

    /* After compiling the function, we probably have gathered some locals,
       which we need to allocate space for. */

    add_local_blocks(func_block);

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

static void compile_type_decl(struct compiler *self,
                              struct node_type_decl *type_decl)
{
    struct type_struct *type;
    struct type *field_type;
    void *thing;

    thing = type_register_resolve(self->types, type_decl->typename);
    if (thing) {
        ce(self, type_decl->head.place, "%s is already defined as a type",
           type_decl->typename);
    }

    type = type_register_alloc_struct(self->types, type_decl->typename);

    for (int i = 0; i < type_decl->n_fields; i++) {
        field_type = make_real_type(self, type_decl->fields[i]->parsed_type);
        type_struct_add_field(type, field_type, type_decl->fields[i]->name);
    }
}

static void compile_global(struct compiler *self, struct node_var_decl *decl)
{
    struct local *global;

    if (find_global(self, decl->name))
        ce(self, decl->head.place, "%s is already declared", decl->name);

    global = alloc_global(self);

    global->type = make_real_type(self, decl->var_type);
    if (!global->type)
        ce(self, decl->var_type->place, "unknown type");
    global->name = string_copy_z(decl->name);
    global->decl = decl;
    global->is_global = true;

    if (!global->type)
        ce(self, decl->var_type->place, "undeclared type");
}

static void add_global_blocks(struct compiler *self)
{
    struct block_global *block;

    for (int i = self->n_globals - 1; i >= 0; i--) {
        block = block_alloc(NULL, BLOCK_GLOBAL);
        block->local = self->globals[i];
        block_add_child(self->file_block, (struct block *) block);
    }
}

void compiler_compile(struct compiler *self)
{
    struct node *func_walker;

    self->file_block = block_alloc(NULL, BLOCK_FILE);
    self->types = ac_alloc(global_ac, sizeof(struct type_register));
    self->globals = NULL;
    self->n_globals = 0;
    self->strings = NULL;
    self->n_strings = 0;
    self->funcs = NULL;
    self->n_funcs = 0;

    add_builtin_types(self);

    /* First, collect all function & type declarations. */

    func_walker = self->tree->child;
    while (func_walker) {
        if (func_walker->type == NODE_FUNC_DECL)
            compile_func_decl(self, (struct node_func_decl *) func_walker);
        if (func_walker->type == NODE_TYPE_DECL)
            compile_type_decl(self, (struct node_type_decl *) func_walker);
        if (func_walker->type == NODE_VAR_DECL)
            compile_global(self, (struct node_var_decl *) func_walker);
        func_walker = func_walker->next;
    }

    /* Walk through the tree again to compile all function definitions. */

    func_walker = self->tree->child;
    while (func_walker) {
        if (func_walker->type == NODE_FUNC_DEF)
            compile_func(self, (struct node_func_def *) func_walker);
        func_walker = func_walker->next;
    }

    /* Add globals. */

    add_global_blocks(self);

    /* We need to remember to add all the STRING blocks to the file. */

    for (int i = 0; i < self->n_strings; i++) {
        block_add_child(self->file_block, (struct block *) self->strings[i]);
    }

    if (self->opts->f_block_tree)
        block_tree_dump(self->file_block);
}
