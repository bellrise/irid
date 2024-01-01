/* Leaf compiler
   Copyright (c) 2023 bellrise */

#include "lc.h"

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

static void emit_store(struct emitter *self, struct block_store *store)
{
    int other_offset;
    int offset;

    offset = store->var->local_block->emit_offset;

    /* Immediate value */
    if (store->value.value_type == VALUE_IMMEDIATE) {
        fprintf(self->out, "    mov r4, bp\n");
        fprintf(self->out, "    sub r4, %d\n", offset);
        fprintf(self->out, "    mov r5, %d\n", store->value.imm_value.value);
        fprintf(self->out, "    store r5, r4\n");
    }

    /* Pointer to string */
    if (store->value.value_type == VALUE_STRING) {
        fprintf(self->out, "    mov r4, bp\n");
        fprintf(self->out, "    sub r4, %d\n", offset);
        fprintf(self->out, "    mov r5, _S%d\n", store->value.string_id_value);
        fprintf(self->out, "    store r5, r4\n");
    }

    /* Copy a variable */
    if (store->value.value_type == VALUE_LOCAL) {
        other_offset = store->value.local_value->local_block->emit_offset;
        fprintf(self->out, "    mov r4, bp\n");
        fprintf(self->out, "    sub r4, %d\n", other_offset);
        fprintf(self->out, "    load r5, r4\n");
        fprintf(self->out, "    mov r4, bp\n");
        fprintf(self->out, "    sub r4, %d\n", offset);
        fprintf(self->out, "    store r5, r4\n");
    }
}

static void store_value_into_register(struct emitter *self, int rid,
                                      struct value *value)
{
    if (value->value_type == VALUE_IMMEDIATE) {
        fprintf(self->out, "    mov r%d, %d\n", rid, value->imm_value.value);
    }

    else if (value->value_type == VALUE_LOCAL) {
        fprintf(self->out, "    mov r4, bp\n");
        fprintf(self->out, "    sub r4, %d\n",
                value->local_value->local_block->emit_offset);
        fprintf(self->out, "    load r%d, r4\n", rid);
    }

    else if (value->value_type == VALUE_STRING) {
        fprintf(self->out, "    mov r4, _S_%d\n", value->string_id_value);
    }

    else {
        die("cannot store value into register");
    }
}

static void emit_call(struct emitter *self, struct block_call *func)
{
    for (int i = 0; i < func->n_args; i++)
        store_value_into_register(self, i, &func->args[i]);
    fprintf(self->out, "    call %s\n", func->call_name);
}

static void emit_asm(struct emitter *self, struct block_asm *_asm)
{
    fprintf(self->out, "    %s\n", _asm->source);
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
        case BLOCK_FUNC_PREAMBLE:
            emit_func_preamble(self, func);
            break;
        case BLOCK_FUNC_EPILOGUE:
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
