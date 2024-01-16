/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <irid/arch.h>

static void store_value_into_register(struct emitter *self, int register_id,
                                      struct value *value);
static void store_register_into_local(struct emitter *self, int register_id,
                                      struct local *local, int offset);

static void emit_func_preamble(struct emitter *self, struct block_func *func)
{
    fprintf(self->out, "    push bp\n");
    fprintf(self->out, "    push r4\n");
    fprintf(self->out, "    push r5\n");
    fprintf(self->out, "    push r6\n");
    fprintf(self->out, "    mov bp, sp\n");
    func->emit_locals_size = 0;
}

static void emit_func_epilogue(struct emitter *self)
{
    fprintf(self->out, "    mov sp, bp\n");
    fprintf(self->out, "    pop r6\n");
    fprintf(self->out, "    pop r5\n");
    fprintf(self->out, "    pop r4\n");
    fprintf(self->out, "    pop bp\n");
}

static void emit_local(struct emitter *self, struct block_func *func,
                       struct block_local *local)
{
    int alloc_size;

    alloc_size = type_size(local->local->type);
    func->emit_locals_size += alloc_size;
    local->emit_offset = func->emit_locals_size;

    fprintf(self->out, "    sub sp, %d  ; %s %s\n", alloc_size,
            type_repr(local->local->type), local->local->name);
}

static const char *register_name(int register_id)
{
    const char *names[] = {
        [R_R0] = "r0", [R_R1] = "r1", [R_R2] = "r2", [R_R3] = "r3",
        [R_R4] = "r4", [R_R5] = "r5", [R_R6] = "r6", [R_R7] = "r7",
    };

    return names[register_id];
};

static void push(struct emitter *self, int register_id)
{
    fprintf(self->out, "    push %s\n", register_name(register_id));
}

static void pop(struct emitter *self, int register_id)
{
    fprintf(self->out, "    pop %s\n", register_name(register_id));
}

static void emit_op(struct emitter *self, int result_register,
                    struct op_value *op)
{
    /* We are on a mission now. Our goal is to place the calculated value into
       the result register. First, we calculate the right side of the operation
       and push that value onto the stack. */

    store_value_into_register(self, result_register, op->right);
    push(self, result_register);

    /* Calculate the left side, and pop the calculated right side into r4. */

    store_value_into_register(self, result_register, op->left);
    pop(self, R_R4);

    if (op->type == OP_ADD) {
        fprintf(self->out, "    add %s, %s\n", register_name(result_register),
                register_name(R_R4));
    }

    else if (op->type == OP_SUB) {
        fprintf(self->out, "    sub %s, %s\n", register_name(result_register),
                register_name(R_R4));
    }

    else if (op->type == OP_MUL) {
        fprintf(self->out, "    mul %s, %s\n", register_name(result_register),
                register_name(R_R4));
    }

    else {
        die("cannot emit math op");
    }
}

static void store_value_addr_into_register(struct emitter *self,
                                           int register_id, struct value *value)
{
    if (value->value_type == VALUE_LOCAL) {
        /* If we want to deref, first load the pointer into a register. */
        if (value->deref) {
            fprintf(self->out, "    mov r4, bp\n");
            fprintf(self->out, "    sub r4, %d\n",
                    value->local_value->local_block->emit_offset);
            fprintf(self->out, "    load %s, r4\n", register_name(register_id));
            fprintf(self->out, "    add %s, %d\n", register_name(register_id),
                    value->local_offset);
        } else {
            fprintf(self->out, "    mov %s, bp\n", register_name(register_id));
            fprintf(self->out, "    sub %s, %d\n", register_name(register_id),
                    value->local_value->local_block->emit_offset
                        - value->local_offset);
        }
    }

    else if (value->value_type == VALUE_GLOBAL) {
        fprintf(self->out, "    mov %s, _G_%s\n", register_name(register_id),
                value->local_value->name);
    }

    else {
        die("cannot store value addr into register");
    }
}

static void store_value_into_register(struct emitter *self, int register_id,
                                      struct value *value)
{
    if (value->value_type == VALUE_IMMEDIATE) {
        fprintf(self->out, "    mov %s, %d\n", register_name(register_id),
                value->imm_value.value);
    }

    else if (value->value_type == VALUE_LOCAL
             || value->value_type == VALUE_GLOBAL) {
        store_value_addr_into_register(self, R_R4, value);

        /* In the case that the value decays into a pointer, store the pointer
           value to the register, skipping the `load` part (which loads the
           value at the pointer).  */

        if (value->decay_into_pointer)
            fprintf(self->out, "    mov %s, r4\n", register_name(register_id));
        else
            fprintf(self->out, "    load %s, r4\n", register_name(register_id));
    }

    else if (value->value_type == VALUE_STRING) {
        fprintf(self->out, "    mov %s, _S_%d\n", register_name(register_id),
                value->string_id_value);
    }

    else if (value->value_type == VALUE_OP) {
        emit_op(self, register_id, &value->op_value);
    }

    else if (value->value_type == VALUE_ADDR) {
        fprintf(self->out, "    mov %s, bp\n", register_name(register_id));
        fprintf(self->out, "    sub %s, %d\n", register_name(register_id),
                value->local_value->local_block->emit_offset
                    + value->local_offset);
    }

    else if (value->value_type == VALUE_INDEX) {
        /* Get the pointer to the value, and then calculate the offset required,
           which means multiplying the index by the size of a single element. */

        store_value_into_register(self, R_R6, value->index_offset);

        /* multiply by the element size */
        fprintf(self->out, "    mul r6, %d\n", value->index_elem_size);

        /* r4 is the base pointer */
        store_value_into_register(self, R_R4, value->index_var);

        /* offset the base pointer */
        fprintf(self->out, "    add r4, r6\n");

        /* read the value at index */
        fprintf(self->out, "    load %s, r4\n", register_name(register_id));

        /* If the value we want to store is a single byte, remember to cut off
           the high-order byte. */

        if (value->index_elem_size == 1) {
            fprintf(self->out, "    and %s, 0xff\n",
                    register_name(register_id));
        }
    }

    else {
        die("cannot store value into register");
    }
}

static void store_register_into_local(struct emitter *self, int register_id,
                                      struct local *local, int offset)
{
    if (local->is_global) {
        fprintf(self->out, "    mov r4, _G_%s\n", local->name);
        fprintf(self->out, "    store %s, r4\n", register_name(register_id));
    }

    else {
        fprintf(self->out, "    mov r4, bp\n");
        fprintf(self->out, "    sub r4, %d\n",
                local->local_block->emit_offset - offset);
        fprintf(self->out, "    store %s, r4\n", register_name(register_id));
    }
}

static void emit_store(struct emitter *self, struct block_store *store)
{
    if (self->opts->f_comment_asm)
        fprintf(self->out, "    ; store %s\n", store->var->name);

    store_value_into_register(self, R_R5, &store->value);
    store_register_into_local(self, R_R5, store->var, store->store_offset);
}

static void emit_call(struct emitter *self, struct block_call *func)
{
    if (self->opts->f_comment_asm)
        fprintf(self->out, "    ; call %s\n", func->call_name);

    for (int i = 0; i < func->n_args; i++) {
        if (self->opts->f_comment_asm)
            fprintf(self->out, "    ; call-arg %d\n", i);
        store_value_into_register(self, R_R0 + i, &func->args[i]);
    }

    fprintf(self->out, "    call %s\n", func->call_name);
}

static void emit_asm(struct emitter *self, struct block_asm *_asm)
{
    fprintf(self->out, "    %s\n", _asm->source);
}

static void emit_label(struct emitter *self, struct block_label *label)
{
    fprintf(self->out, "@%s:\n", label->label);
}

static void emit_jmp(struct emitter *self, struct block_jmp *jmp)
{
    if (jmp->type == JMP_ALWAYS)
        fprintf(self->out, "    jmp @%s\n", jmp->dest);

    else if (jmp->type == JMP_EQ)
        fprintf(self->out, "    jeq @%s\n", jmp->dest);

    else if (jmp->type == JMP_NEQ) {
        fprintf(self->out, "    cfs\n");
        fprintf(self->out, "    jeq @%s\n", jmp->dest);
    }

    else
        die("cannot emit jump, unknown flag");
}

static void emit_cmp(struct emitter *self, struct block_cmp *cmp)
{
    if (self->opts->f_comment_asm)
        fprintf(self->out, "    ; cmp\n");

    store_value_into_register(self, R_R5, &cmp->left);

    if (self->opts->f_cmp_literal && cmp->right.value_type == VALUE_IMMEDIATE) {
        fprintf(self->out, "    cmp r5, %d\n", cmp->right.imm_value.value);
        return;
    }

    store_value_into_register(self, R_R6, &cmp->right);
    fprintf(self->out, "    cmp r5, r6\n");
}

static void emit_store_return(struct emitter *self,
                              struct block_store *store_return)
{
    store_value_into_register(self, R_R0, &store_return->value);
}

static void emit_store_result(struct emitter *self,
                              struct block_store *store_return)
{
    store_register_into_local(self, R_R0, store_return->var,
                              store_return->store_offset);
}

static void emit_store_arg(struct emitter *self,
                           struct block_store_arg *store_arg)
{
    store_register_into_local(self, R_R0 + store_arg->arg, store_arg->local, 0);
}

static void emit_func(struct emitter *self, struct block_func *func)
{
    struct block *block;

    fputc('\n', self->out);

    if (func->exported)
        fprintf(self->out, ".export %s\n", func->label);
    fprintf(self->out, "%s:\n", func->label);

    block = func->head.child;
    while (block) {
        switch (block->type) {
        case BLOCK_PREAMBLE:
            emit_func_preamble(self, func);
            break;
        case BLOCK_LOCAL:
            emit_local(self, func, (struct block_local *) block);
            break;
        case BLOCK_EPILOGUE:
            emit_func_epilogue(self);
            break;
        case BLOCK_STORE:
            emit_store(self, (struct block_store *) block);
            break;
        case BLOCK_CALL:
            emit_call(self, (struct block_call *) block);
            break;
        case BLOCK_ASM:
            emit_asm(self, (struct block_asm *) block);
            break;
        case BLOCK_LABEL:
            emit_label(self, (struct block_label *) block);
            break;
        case BLOCK_JMP:
            emit_jmp(self, (struct block_jmp *) block);
            break;
        case BLOCK_STORE_RETURN:
            emit_store_return(self, (struct block_store *) block);
            break;
        case BLOCK_STORE_RESULT:
            emit_store_result(self, (struct block_store *) block);
            break;
        case BLOCK_STORE_ARG:
            emit_store_arg(self, (struct block_store_arg *) block);
            break;
        case BLOCK_CMP:
            emit_cmp(self, (struct block_cmp *) block);
            break;
        default:
            die("don't know how to emit %s", block_name(block));
        }

        block = block->next;
    }

    fprintf(self->out, "    ret\n");
}

static void emit_string(struct emitter *self, struct block_string *string)
{
    fprintf(self->out, "_S_%d:\n", string->string_id);
    fprintf(self->out, ".string \"%s\"\n", string->value);
}

static void emit_global(struct emitter *self, struct block_global *global)
{
    fprintf(self->out, "_G_%s:\n", global->local->name);
    fprintf(self->out, ".resv %d\n", type_size(global->local->type));
}

void emitter_emit(struct emitter *self, struct options *opts)
{
    struct block *block;

    self->out = fopen(opts->output, "w");
    if (!self->out)
        die("failed to open result file");

    if (self->file_block->type != BLOCK_FILE)
        die("expected BLOCK_FILE in emitter_emit");

    fprintf(self->out, "; Generated by irid-lc\n");

    if (opts->set_origin)
        fprintf(self->out, "\n.org %d\n", opts->origin);

    block = self->file_block->child;
    while (block) {
        switch (block->type) {
        case BLOCK_FUNC:
            emit_func(self, (struct block_func *) block);
            break;
        case BLOCK_STRING:
            emit_string(self, (struct block_string *) block);
            break;
        case BLOCK_GLOBAL:
            emit_global(self, (struct block_global *) block);
            break;
        default:
            die("unexpected block: %s", block_name(block));
        }

        block = block->next;
    }
}
