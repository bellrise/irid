/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "compiler.h"

struct type *make_real_type(struct compiler *self,
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

struct local *alloc_local(struct block_func *func)
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

struct local *find_local(struct block_func *func, const char *name)
{
    for (int i = 0; i < func->n_locals; i++) {
        if (!strcmp(func->locals[i]->name, name))
            return func->locals[i];
    }

    return NULL;
}

struct local *find_global(struct compiler *self, const char *name)
{
    for (int i = 0; i < self->n_globals; i++) {
        if (!strcmp(self->globals[i]->name, name))
            return self->globals[i];
    }

    return NULL;
}

struct func_sig *find_func(struct compiler *self, char *name)
{
    for (int i = 0; i < self->n_funcs; i++) {
        if (!strcmp(self->funcs[i]->name, name))
            return self->funcs[i];
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

struct type *resolve_value_type(struct compiler *self, struct value *val)
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

        if (resolved_type->type == TYPE_ARRAY) {
            resolved_type = ((struct type_array *) resolved_type)->base_type;
            break;
        }

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

void place_node(struct compiler *self, struct block_func *func_node,
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
