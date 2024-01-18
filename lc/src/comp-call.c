/* Leaf compiler
   Copyright (c) 2024 bellrise */

#include "compiler.h"

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

void compile_call(struct compiler *self, struct block_func *func,
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
