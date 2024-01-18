/* Leaf compiler
   Copyright (c) 2024 bellrise */

#include "compiler.h"

static struct local *resolve_local(struct compiler *self,
                                   struct block_func *func,
                                   struct node_label *label)
{
    struct local *local;

    local = find_local(func, label->name);
    if (!local)
        local = find_global(self, label->name);

    if (!local)
        ce(self, label->head.place, "undeclared variable");

    return local;
}

static struct type *get_underlying_type(struct type *type)
{
    if (type->type == TYPE_POINTER)
        return ((struct type_pointer *) type)->base_type;

    if (type->type == TYPE_ARRAY)
        return ((struct type_array *) type)->base_type;

    return type;
}

static struct local *resolve_field(struct compiler *self,
                                   struct block_func *func,
                                   struct node_field *field, int *offset)
{
    struct node_label *field_label;
    struct node_label *var_label;
    struct type_struct *var_type;
    struct local *local;

    var_label = (struct node_label *) field->head.child;
    field_label = (struct node_label *) var_label->head.next;

    if (var_label->head.type != NODE_LABEL)
        ce(self, var_label->head.place, "cannot access field of this");

    local = resolve_local(self, func, var_label);
    var_type = (struct type_struct *) get_underlying_type(local->type);

    if (var_type->head.type != TYPE_STRUCT) {
        cw(self, local->decl->head.place, "declared here");
        ce(self, var_label->head.place,
           "cannot access field of non-structure type");
    }

    if (field_label->head.type != NODE_LABEL)
        ce(self, var_label->head.place, "fields can only be symbols");

    if (!type_struct_find_field(var_type, field_label->name)) {
        ce(self, var_label->head.place, "no such field in %s",
           var_type->head.name);
    }

    *offset = type_struct_field_offset(var_type, field_label->name);

    return local;
}

static void resolve_index(struct compiler *self, struct block_func *func,
                          struct node *index_node,
                          struct block_store *store_block)
{
    struct node *var_node;
    struct node *index;

    var_node = index_node->child;
    index = var_node->next;

    /* What we're indexing into. */

    if (var_node->type == NODE_LITERAL) {
        ce(self, var_node->place, "cannot index into literal");
    }

    else if (var_node->type == NODE_LABEL) {
        store_block->local =
            resolve_local(self, func, (struct node_label *) var_node);
    }

    else if (var_node->type == NODE_FIELD) {
        store_block->local =
            resolve_field(self, func, (struct node_field *) var_node,
                          &store_block->store_offset);
    }

    else {
        ce(self, var_node->place, "cannot index into this expression");
    }

    /* The index itself. */

    store_block->index = ac_alloc(global_ac, sizeof(struct value));
    convert_node_into_value(self, func, store_block->index, index);
}

static void assign_var(struct compiler *self, struct block_func *func,
                       struct block_store *store_block, struct node *var_node)
{
    /* We can assign to a variable, field or index. Note that the index may also
       contain the field access. */

    if (var_node->type == NODE_LABEL) {
        store_block->local =
            resolve_local(self, func, (struct node_label *) var_node);
    }

    else if (var_node->type == NODE_FIELD) {
        store_block->local =
            resolve_field(self, func, (struct node_field *) var_node,
                          &store_block->store_offset);
    }

    else if (var_node->type == NODE_INDEX) {
        resolve_index(self, func, var_node, store_block);
    }
}

void compile_assign(struct compiler *self, struct block_func *func,
                    struct node *assign_node)
{
    struct block_store *store_block;
    struct node *possible_value;

    /* Create a new BLOCK_STORE instruction, but don't assign it to the parent
       just yet - converting a node into a value may take more steps than just
       one. */

    store_block = block_alloc(NULL, BLOCK_STORE);

    assign_var(self, func, store_block, assign_node->child);

    possible_value = assign_node->child->next;
    convert_node_into_value(self, func, &store_block->value, possible_value);

    block_add_child((struct block *) func, (struct block *) store_block);
}
