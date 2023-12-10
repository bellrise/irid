/* ld.c - Irid linker
   Copyright (c) 2023 bellrise */

#include "ld.h"

int main(int argc, char **argv)
{
    struct options opts;
    struct ld_object *object;
    struct ld_linker linker;

    opt_set_defaults(&opts);
    opt_parse(&opts, argc, argv);

    if (opts.inputs.size > 1)
        die("currently only supports a single object file");
    if (opts.inputs.size == 0)
        die("missing input file, see --help");

    object = ld_object_new(NULL);
    ld_object_from_file(object, opts.inputs.strings[0]);

    if (opts.dump_symbols) {
        ld_object_dump(object);
    } else {
        linker.first_object = object;
        ld_linker_link(&linker, opts.output);
    }

    ld_object_free(object);
    strlist_free(&opts.inputs);
}
