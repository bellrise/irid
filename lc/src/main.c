/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdio.h>
#include <stdlib.h>

struct allocator *global_ac;

int main(int argc, char **argv)
{
    struct allocator main_allocator;
    struct compiler compiler;
    struct emitter emitter;
    struct tokens *tokens;
    struct parser *parser;
    struct options opts;
    char *source;

    allocator_init(&main_allocator);
    global_ac = &main_allocator;

    opt_set_defaults(&opts);
    opt_parse(&opts, argc, argv);
    opt_add_missing_opts(&opts);

    tokens = tokens_new();
    source = file_read(opts.input);
    source = preproc_remove_comments(source);
    source = preproc_replace_tabs(source);

    tokens->source = source;
    tokens->source_name = opts.input;
    tokens_tokenize(tokens);

    parser = parser_new();
    parser->tokens = tokens;
    parser_parse(parser);

    if (opts.f_node_tree)
        node_tree_dump(parser->tree);

    compiler.tree = parser->tree;
    compiler.file_block = NULL;
    compiler.tokens = tokens;
    compiler.opts = &opts;
    compiler_compile(&compiler);

    emitter.file_block = compiler.file_block;
    emitter.opts = &opts;
    emitter_emit(&emitter, &opts);

    fclose(emitter.out);
    allocator_free_all(global_ac);

    free(source);
}
