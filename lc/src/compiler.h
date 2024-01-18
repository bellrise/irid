/* Leaf compiler
   Copyright (c) 2024 bellrise */

#ifndef LC_COMP_H
#define LC_COMP_H

#include "lc.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void ce(struct compiler *self, struct tok *place, const char *fmt, ...);
void cw(struct compiler *self, struct tok *place, const char *fmt, ...);
void cn(struct compiler *self, struct tok *place, const char *fmt, ...);
void ceh(struct compiler *self, struct tok *place, const char *help_msg,
         const char *fmt, ...);

struct type *resolve_value_type(struct compiler *self, struct value *val);
void convert_node_into_value(struct compiler *self, struct block_func *func,
                             struct value *value, struct node *node);
struct func_sig *find_func(struct compiler *self, char *name);
struct type *make_real_type(struct compiler *self,
                            struct parsed_type *parsed_type);
void place_node(struct compiler *self, struct block_func *func_node,
                struct node *this_node);
struct local *alloc_local(struct block_func *func);
struct local *find_local(struct block_func *func, const char *name);
struct local *find_global(struct compiler *self, const char *name);

void compile_assign(struct compiler *self, struct block_func *func,
                    struct node *assign_node);
void compile_asm_call(struct compiler *self, struct block_func *func,
                      struct node *call);
void compile_call(struct compiler *self, struct block_func *func,
                  struct node *call);
void compile_func(struct compiler *self, struct node_func_def *func);
void compile_func_decl(struct compiler *self, struct node_func_decl *func_decl);
void compile_loop(struct compiler *self, struct block_func *func_block,
                  struct node_loop *loop_node);

#endif /* LC_COMP_H */
