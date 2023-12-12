/* ld.c - Irid linker
   Copyright (c) 2023 bellrise */

#include "ld.h"

int main(int argc, char **argv)
{
    struct ld_object *first_object;
    struct ld_object *object;
    struct ld_linker *linker;
    struct options opts;

    opt_set_defaults(&opts);
    opt_parse(&opts, argc, argv);
    linker = NULL;

    if (opts.inputs.size == 0)
        die("missing input file, see --help");

    /* Load objects from files. */

    first_object = ld_object_new(NULL);
    ld_object_from_file(first_object, opts.inputs.strings[0]);

    for (size_t i = 1; i < opts.inputs.size; i++) {
        object = ld_object_new(first_object);
        ld_object_from_file(object, opts.inputs.strings[i]);
    }

    /* Dump symbols */

    if (opts.dump_symbols) {
        object = first_object;
        while (object) {
            ld_object_dump_header(object);
            object = object->next;
        }
        goto end;
    }

    /* Link objects */

    linker = ld_linker_new();
    linker->first_object = first_object;
    linker->verbose = opts.verbose;
    ld_linker_link(linker, opts.output);

end:
    ld_object_free(first_object);
    ld_linker_free(linker);
    strlist_free(&opts.inputs);
}
