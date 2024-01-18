/* Leaf compiler
   Copyright (c) 2024 bellrise */

#include "compiler.h"

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
    store_block->local = result_local;

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

void convert_node_into_value(struct compiler *self, struct block_func *func,
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
        if (type->type != TYPE_POINTER && type->type != TYPE_ARRAY)
            ce(self, node->place, "cannot index into non-pointer variable");

        value->index_elem_size =
            type_size(((struct type_pointer *) type)->base_type);
        value->deref = type->type == TYPE_POINTER;

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
