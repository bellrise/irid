/* Leaf compiler
   Copyright (c) 2024 bellrise */

#include "compiler.h"

void ce(struct compiler *self, struct tok *place, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    source_error(self->tokens, place->pos, place->len, 0, NULL, NULL, "error",
                 "\033[31m", fmt, args);
    va_end(args);
    exit(1);
}

void cw(struct compiler *self, struct tok *place, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    source_error(self->tokens, place->pos, place->len, 0, NULL, NULL, "warning",
                 "\033[35m", fmt, args);
    va_end(args);
}

void ceh(struct compiler *self, struct tok *place, const char *help_msg,
         const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    source_error(self->tokens, place->pos, place->len, 0, NULL, help_msg,
                 "error", "\033[31m", fmt, args);
    va_end(args);
    exit(1);
}
