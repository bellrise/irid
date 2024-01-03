/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <irid/arch.h>

static void store_value_into_register(struct emitter *self, int register_id,
                                      struct value *value);
static void store_register_into_local(struct emitter *self, int register_id,
                                      struct block_local *local);

static void emit_func_preamble(struct emitter *self, struct block_func *func)
{
    fprintf(self->out, "    push bp\n");
    fprintf(self->out, "    push r4\n");
    fprintf(self->out, "    push r5\n");
    fprintf(self->out, "    mov bp, sp\n");
    func->emit_locals_size = 0;
}

static void emit_func_epilogue(struct emitter *self)
{
    fprintf(self->out, "    mov sp, bp\n");
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

static void store_value_into_register(struct emitter *self, int register_id,
                                      struct value *value)
{
    if (value->value_type == VALUE_IMMEDIATE) {
        fprintf(self->out, "    mov %s, %d\n", register_name(register_id),
                value->imm_value.value);
    }

    else if (value->value_type == VALUE_LOCAL) {
        fprintf(self->out, "    mov r4, bp\n");
        fprintf(self->out, "    sub r4, %d\n",
                value->local_value->local_block->emit_offset);
        fprintf(self->out, "    load %s, r4\n", register_name(register_id));
    }

    else if (value->value_type == VALUE_STRING) {
        fprintf(self->out, "    mov r4, _S_%d\n", value->string_id_value);
    }

    else if (value->value_type == VALUE_OP) {
        emit_op(self, register_id, &value->op_value);
    }

    else {
        die("cannot store value into register");
    }
}

static void store_register_into_local(struct emitter *self, int register_id,
                                      struct block_local *local)
{
    fprintf(self->out, "    mov r4, bp\n");
    fprintf(self->out, "    sub r4, %d\n", local->emit_offset);
    fprintf(self->out, "    store %s, r4\n", register_name(register_id));
}

static void emit_store(struct emitter *self, struct block_store *store)
{
    store_value_into_register(self, R_R5, &store->value);
    store_register_into_local(self, R_R5, store->var->local_block);
}

static void emit_call(struct emitter *self, struct block_call *func)
{
    for (int i = 0; i < func->n_args; i++)
        store_value_into_register(self, R_R0 + i, &func->args[i]);
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
    fprintf(self->out, "    jmp @%s\n", jmp->dest);
}

static void emit_store_return(struct emitter *self,
                              struct block_store *store_return)
{
    store_value_into_register(self, R_R0, &store_return->value);
}

static void emit_store_result(struct emitter *self,
                              struct block_store *store_return)
{
    store_register_into_local(self, R_R0, store_return->var->local_block);
}

static void emit_store_arg(struct emitter *self,
                           struct block_store_arg *store_arg)
{
    store_register_into_local(self, R_R0 + store_arg->arg,
                              store_arg->local->local_block);
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
        case BLOCK_EPILOGUE:
            emit_func_epilogue(self);
            break;
        case BLOCK_LOCAL:
            emit_local(self, func, (struct block_local *) block);
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
        default:
            die("don't know how to emit %s", block_name(block));
        }

        block = block->next;
    }

    fprintf(self->out, "    ret\n");
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
        if (block->type != BLOCK_FUNC)
            die("only BLOCK_FUNC allowed at file top-level");

        emit_func(self, (struct block_func *) block);
        block = block->next;
    }
}
